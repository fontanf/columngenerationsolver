/**
 * Multiple knapsack problem.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/multipleknapsack.hpp
 *
 * The linear programming formulation of the problem based on Dantzig–Wolfe
 * decomposition is written as follows:
 *
 * Variables:
 * - yᵢᵏ ∈ {0, 1} representing a set of items for knapsack i.
 *   yᵢᵏ = 1 iff the corresponding set of items is selected for knapsack i.
 *   xⱼᵢᵏ = 1 iff yᵢᵏ contains item j, otherwise 0.
 *
 * Program:
 *
 * max ∑ᵢ ∑ₖ (∑ⱼ pⱼ xⱼᵢᵏ) yᵢᵏ
 *                                      Note that (∑ⱼ pⱼ xⱼᵢᵏ) is a constant.
 *
 * 1 <= ∑ₖ yᵢᵏ <= 1        for all knapsack i
 *                       (not more than 1 packing selected for each knapsack)
 *                                                         Dual variables: uᵢ
 * 0 <= ∑ᵢ∑ₖ xⱼᵢᵏ yᵢᵏ <= 1   for all items j
 *                                          (each item selected at most once)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of positive reduced cost.
 * The reduced cost of a variable yᵢᵏ is given by:
 * rc(yᵢᵏ) = ∑ⱼ pⱼ xⱼᵢᵏ - uᵢ - ∑ⱼ xⱼᵢᵏ vⱼ
 *         = ∑ⱼ (pⱼ - vⱼ) xⱼᵢᵏ - uᵢ
 *
 * Therefore, finding a variable of maximum reduced cost reduces to solving
 * m Knapsack Problems with items with profit (pⱼ - vⱼ).
 *
 */

#pragma once

#include "columngenerationsolver/commons.hpp"

#include "orproblems/multipleknapsack.hpp"

#include "knapsacksolver/knapsack/instance_builder.hpp"
#include "knapsacksolver/knapsack/algorithms/dynamic_programming_primal_dual.hpp"

namespace columngenerationsolver
{
namespace multipleknapsack
{

using namespace orproblems::multipleknapsack;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        fixed_items_(instance.number_of_items()),
        fixed_knapsacks_(instance.number_of_knapsacks())
    {  }

    virtual inline std::vector<std::shared_ptr<const Column>> initialize_pricing(
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns);

    virtual inline std::vector<std::shared_ptr<const Column>> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<int8_t> fixed_items_;

    std::vector<int8_t> fixed_knapsacks_;

    std::vector<ItemId> kp2mkp_;

};

inline columngenerationsolver::Model get_model(const Instance& instance)
{
    columngenerationsolver::Model model;

    model.objective_sense = optimizationtools::ObjectiveDirection::Maximize;
    model.column_lower_bound = 0;
    model.column_upper_bound = 1;

    // Add knapsack constraints.
    for (KnapsackId knapsack_id = 0;
            knapsack_id < instance.number_of_knapsacks();
            ++knapsack_id) {
        Row row;
        row.lower_bound = 1;
        row.upper_bound = 1;
        row.coefficient_lower_bound = 0;
        row.coefficient_upper_bound = 1;
        model.rows.push_back(row);
    }

    // Add item constraints.
    for (ItemId item_id = 0;
            item_id < instance.number_of_items();
            ++item_id) {
        Row row;
        row.lower_bound = 0;
        row.upper_bound = 1;
        row.coefficient_lower_bound = 0;
        row.coefficient_upper_bound = 1;
        model.rows.push_back(row);
    }

    // Dummy column objective coefficient.
    model.dummy_column_objective_coefficient = -10 * instance.item(0).profit;

    // Pricing solver.
    model.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));

    return model;
}

std::vector<std::shared_ptr<const Column>> PricingSolver::initialize_pricing(
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns)
{
    std::fill(fixed_items_.begin(), fixed_items_.end(), -1);
    std::fill(fixed_knapsacks_.begin(), fixed_knapsacks_.end(), -1);
    for (const auto& p: fixed_columns) {
        const Column& column = *(p.first);
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (const LinearTerm& element: column.elements) {
            if (element.coefficient < 0.5)
                continue;
            if (element.row < instance_.number_of_knapsacks()) {
                fixed_knapsacks_[element.row] = 1;
            } else {
                fixed_items_[element.row - instance_.number_of_knapsacks()] = 1;
            }
        }
    }
    return {};
}

std::vector<std::shared_ptr<const Column>> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    std::vector<std::shared_ptr<const Column>> columns;
    for (KnapsackId knapsack_id = 0;
            knapsack_id < instance_.number_of_knapsacks();
            ++knapsack_id) {
        if (fixed_knapsacks_[knapsack_id] == 1)
            continue;

        // Build subproblem instance.
        knapsacksolver::knapsack::InstanceFromFloatProfitsBuilder kp_instance_builder;
        Weight capacity = instance_.capacity(knapsack_id);
        kp2mkp_.clear();
        for (ItemId item_id = 0;
                item_id < instance_.number_of_items();
                ++item_id) {
            if (fixed_items_[item_id] == 1)
                continue;
            const Item& item = instance_.item(item_id);
            double profit = item.profit
                - duals[instance_.number_of_knapsacks() + item_id];
            if (profit <= 0 || item.weight > instance_.capacity(knapsack_id))
                continue;
            kp_instance_builder.add_item(profit, item.weight);
            kp2mkp_.push_back(item_id);
        }
        kp_instance_builder.set_capacity(capacity);
        knapsacksolver::knapsack::Instance kp_instance = kp_instance_builder.build();

        // Solve subproblem instance.
        knapsacksolver::knapsack::DynamicProgrammingPrimalDualParameters kp_parameters;
        kp_parameters.verbosity_level = 0;
        auto kp_output = knapsacksolver::knapsack::dynamic_programming_primal_dual(kp_instance, kp_parameters);

        // Retrieve column.
        Column column;
        column.elements.push_back({knapsack_id, 1});
        for (knapsacksolver::knapsack::ItemId kp_item_id = 0;
                kp_item_id < kp_instance.number_of_items();
                ++kp_item_id) {
            if (kp_output.solution.contains(kp_item_id)) {
                ItemId item_id = kp2mkp_[kp_item_id];
                LinearTerm element;
                element.row = instance_.number_of_knapsacks() + item_id;
                element.coefficient = 1;
                column.elements.push_back(element);
                column.objective_coefficient += instance_.item(item_id).profit;
            }
        }
        columns.push_back(std::shared_ptr<const Column>(new Column(column)));
    }
    return columns;
}

inline void write_solution(
        const Instance& instance,
        const Solution& solution,
        const std::string& certificate_path)
{
    std::ofstream file(certificate_path);
    if (!file.good()) {
        throw std::runtime_error(
                "Unable to open file \"" + certificate_path + "\".");
    }

    std::vector<std::vector<ItemId>> sol(instance.number_of_knapsacks());
    for (auto colval: solution.columns()) {
        const Column& column = *(colval.first);
        //Value value = colval.second;
        // Get the knapsack id.
        KnapsackId knapsack_id = -1;
        for (const LinearTerm& element: column.elements) {
            if (element.coefficient > 0.5
                    && element.row < instance.number_of_knapsacks()) {
                knapsack_id = element.row;
            }
        }
        // Get the items.
        for (const LinearTerm& element: column.elements) {
            if (element.coefficient > 0.5
                    && element.row >= instance.number_of_knapsacks()) {
                ItemId item_id = element.row - instance.number_of_knapsacks();
                sol[knapsack_id].push_back(item_id);
            }
        }
    }
    // Write.
    for (KnapsackId knapsack_id = 0;
            knapsack_id < instance.number_of_knapsacks();
            ++knapsack_id) {
        file << sol[knapsack_id].size() << std::endl;
        for (ItemId item_id: sol[knapsack_id])
            file << " " << item_id;
        file << std::endl;
    }
}

}
}
