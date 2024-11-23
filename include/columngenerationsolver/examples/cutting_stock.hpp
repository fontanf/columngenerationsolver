/**
 * Cutting stock problem.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/cutting_stock.hpp
 *
 * The linear programming formulation of the problem based on Dantzig–Wolfe
 * decomposition is written as follows:
 *
 * Variables:
 * - yᵏ ∈ {0, qmax} representing a set of items fitting into a bin.
 *   yᵏ = q iff the corresponding set of items is selected q times.
 *   xⱼᵏ = q iff yᵏ contains q copies of item type j, otherwise 0.
 *
 * Program:
 *
 * min ∑ₖ yᵏ
 *
 * qⱼ <= ∑ₖ xⱼᵏ yᵏ <= qⱼ     for all items j
 *                                      (each item selected exactly qⱼ times)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of negative reduced cost.
 * The reduced cost of a variable yᵏ is given by:
 * rc(yᵏ) = 1 - ∑ⱼ xⱼᵏ vⱼ
 *        = - ∑ⱼ vⱼ xⱼᵏ + 1
 *
 * Therefore, finding a variable of minimum reduced cost reduces to solving
 * a Bounded Knapsack Problem with items with profit vⱼ.
 *
 */

#pragma once

#include "columngenerationsolver/commons.hpp"

#include "orproblems/packing/cutting_stock.hpp"

#include "knapsacksolver/knapsack/instance_builder.hpp"
#include "knapsacksolver/knapsack/algorithms/dynamic_programming_bellman.hpp"
#include "knapsacksolver/knapsack/algorithms/dynamic_programming_primal_dual.hpp"

namespace columngenerationsolver
{
namespace cutting_stock
{

using namespace orproblems::cutting_stock;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        filled_demands_(instance.number_of_item_types())
    { }

    inline virtual std::vector<std::shared_ptr<const Column>> initialize_pricing(
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns);

    inline virtual PricingOutput solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<Demand> filled_demands_;

    std::vector<ItemTypeId> kp2csp_;

};

inline columngenerationsolver::Model get_model(const Instance& instance)
{
    columngenerationsolver::Model model;

    model.objective_sense = optimizationtools::ObjectiveDirection::Minimize;

    // Rows.
    for (ItemTypeId item_type_id = 0;
            item_type_id < instance.number_of_item_types();
            ++item_type_id) {
        const ItemType& item_type = instance.item_type(item_type_id);
        Row row;
        row.lower_bound = item_type.demand;
        row.upper_bound = item_type.demand;
        row.coefficient_lower_bound = 0;
        row.coefficient_upper_bound = item_type.demand;
        model.rows.push_back(row);
    }

    // Pricing solver.
    model.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));

    return model;
}

std::vector<std::shared_ptr<const Column>> PricingSolver::initialize_pricing(
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns)
{
    std::fill(filled_demands_.begin(), filled_demands_.end(), 0);
    for (const auto& p: fixed_columns) {
        const Column& column = *(p.first);
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (const LinearTerm& element: column.elements)
            filled_demands_[element.row] += value * element.coefficient;
    }
    return {};
}

PricingSolver::PricingOutput PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    PricingOutput output;
    Value reduced_cost_bound = 0.0;

    // Build subproblem instance.
    kp2csp_.clear();
    knapsacksolver::knapsack::InstanceFromFloatProfitsBuilder kp_instance_builder;
    for (ItemTypeId item_type_id = 0;
            item_type_id < instance_.number_of_item_types();
            ++item_type_id) {
        const ItemType& item_type = instance_.item_type(item_type_id);
        double profit = duals[item_type_id];
        if (profit <= 0)
            continue;
        for (Demand q = filled_demands_[item_type_id];
                q < item_type.demand;
                ++q) {
            kp_instance_builder.add_item(profit, item_type.weight);
            kp2csp_.push_back(item_type_id);
        }
    }
    kp_instance_builder.set_capacity(instance_.capacity());
    knapsacksolver::knapsack::Instance kp_instance = kp_instance_builder.build();

    // Solve subproblem instance.
    knapsacksolver::knapsack::Output kp_output(kp_instance);
    if (kp_instance.capacity() <= 1e3) {
        knapsacksolver::knapsack::Parameters kp_parameters;
        kp_parameters.verbosity_level = 0;
        kp_output = knapsacksolver::knapsack::dynamic_programming_bellman_array_all(
                kp_instance,
                kp_parameters);
    } else {
        knapsacksolver::knapsack::DynamicProgrammingPrimalDualParameters kp_parameters;
        kp_parameters.verbosity_level = 0;
        kp_output = knapsacksolver::knapsack::dynamic_programming_primal_dual(
                kp_instance,
                kp_parameters);
    }

    // Retrieve column.
    Column column;
    column.objective_coefficient = 1;
    std::vector<Demand> demands(instance_.number_of_item_types(), 0);
    for (knapsacksolver::knapsack::ItemId kp_item_id = 0;
            kp_item_id < kp_instance.number_of_items();
            ++kp_item_id) {
        if (kp_output.solution.contains(kp_item_id))
            demands[kp2csp_[kp_item_id]]++;
    }
    for (ItemTypeId item_type_id = 0;
            item_type_id < instance_.number_of_item_types();
            ++item_type_id) {
        if (demands[item_type_id] > 0) {
            LinearTerm element;
            element.row = item_type_id;
            element.coefficient = demands[item_type_id];
            column.elements.push_back(element);
        }
    }
    output.columns.push_back(std::shared_ptr<const Column>(new Column(column)));
    output.overcost = instance_.total_demand() * std::min(0.0, columngenerationsolver::compute_reduced_cost(*output.columns.front(), duals));
    return output;
}

inline void write_solution(
        const Solution& solution,
        const std::string& certificate_path)
{
    std::ofstream file(certificate_path);
    if (!file.good()) {
        throw std::runtime_error(
                "Unable to open file \"" + certificate_path + "\".");
    }

    file << solution.columns().size() << std::endl;
    for (auto colval: solution.columns()) {
        const Column& column = *(colval.first);
        Value value = colval.second;
        file << std::round(value)
            << " " << column.elements.size() << "  ";
        for (const LinearTerm& element: column.elements) {
            file << "  " << element.row
                << " " << std::round(element.coefficient);
        }
        file << std::endl;
    }
}

}
}
