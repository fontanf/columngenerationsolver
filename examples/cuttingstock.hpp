/**
 * Cutting stock problem.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/cuttingstock.hpp
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

#include "orproblems/cuttingstock.hpp"

#include "knapsacksolver/algorithms/dynamic_programming_primal_dual.hpp"

namespace columngenerationsolver
{

namespace cuttingstock
{

using namespace orproblems::cuttingstock;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        filled_demands_(instance.number_of_item_types())
    { }

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<Demand> filled_demands_;

    std::vector<ItemTypeId> kp2csp_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    ItemTypeId n = instance.number_of_item_types();
    columngenerationsolver::Parameters p(n);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Min;
    p.column_lower_bound = 0;
    p.column_upper_bound = instance.maximum_demand();
    // Row bounds.
    for (ItemTypeId item_type_id = 0;
            item_type_id < n;
            ++item_type_id) {
        const ItemType& item_type = instance.item_type(item_type_id);
        p.row_lower_bounds[item_type_id] = item_type.demand;
        p.row_upper_bounds[item_type_id] = item_type.demand;
        p.row_coefficient_lower_bounds[item_type_id] = 0;
        p.row_coefficient_upper_bounds[item_type_id] = item_type.demand;
    }
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = 2 * instance.maximum_demand();
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));
    return p;
}

std::vector<ColIdx> PricingSolver::initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns)
{
    std::fill(filled_demands_.begin(), filled_demands_.end(), 0);
    for (auto p: fixed_columns) {
        const Column& column = columns[p.first];
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
            RowIdx row_index = column.row_indices[row_pos];
            Value row_coefficient = column.row_coefficients[row_pos];
            filled_demands_[row_index] += value * row_coefficient;
        }
    }
    return {};
}

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    knapsacksolver::Profit mult = 10000;

    // Build subproblem instance.
    knapsacksolver::Instance kp_instance;
    kp_instance.set_capacity(instance_.capacity());
    kp2csp_.clear();
    for (ItemTypeId item_type_id = 0;
            item_type_id < instance_.number_of_item_types();
            ++item_type_id) {
        const ItemType& item_type = instance_.item_type(item_type_id);
        knapsacksolver::Profit profit = std::floor(mult * duals[item_type_id]);
        if (profit <= 0)
            continue;
        for (Demand q = filled_demands_[item_type_id];
                q < item_type.demand;
                ++q) {
            kp_instance.add_item(item_type.weight, profit);
            kp2csp_.push_back(item_type_id);
        }
    }

    // Solve subproblem instance.
    knapsacksolver::DynamicProgrammingPrimalDualOptionalParameters kp_parameters;
    //kp_parameters.info.set_verbose(true);
    auto kp_output = knapsacksolver::dynamic_programming_primal_dual(
            kp_instance,
            kp_parameters);

    // Retrieve column.
    Column column;
    column.objective_coefficient = 1;
    std::vector<Demand> demands(instance_.number_of_item_types(), 0);
    for (knapsacksolver::ItemIdx kp_item_id = 0;
            kp_item_id < kp_instance.number_of_items();
            ++kp_item_id) {
        if (kp_output.solution.contains_idx(kp_item_id))
            demands[kp2csp_[kp_item_id]]++;
    }
    for (ItemTypeId item_type_id = 0;
            item_type_id < instance_.number_of_item_types();
            ++item_type_id) {
        if (demands[item_type_id] > 0) {
            column.row_indices.push_back(item_type_id);
            column.row_coefficients.push_back(demands[item_type_id]);
        }
    }
    return {column};
}

}

}
