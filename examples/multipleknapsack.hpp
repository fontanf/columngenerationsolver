/**
 * Multiple Knapsack Problem.
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
#include "knapsacksolver/algorithms/minknap.hpp"

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

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<int8_t> fixed_items_;
    std::vector<int8_t> fixed_knapsacks_;

    std::vector<ItemId> kp2mkp_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    KnapsackId m = instance.number_of_knapsacks();
    ItemId n = instance.number_of_items();
    columngenerationsolver::Parameters p(m + n);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Max;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row lower bounds.
    std::fill(p.row_lower_bounds.begin(), p.row_lower_bounds.begin() + m, 1);
    std::fill(p.row_lower_bounds.begin() + m, p.row_lower_bounds.end(), 0);
    // Row upper bounds.
    std::fill(p.row_upper_bounds.begin(), p.row_upper_bounds.begin() + m, 1);
    std::fill(p.row_upper_bounds.begin() + m, p.row_upper_bounds.end(), 1);
    // Row coefficent lower bounds.
    std::fill(p.row_coefficient_lower_bounds.begin(), p.row_coefficient_lower_bounds.end(), 0);
    // Row coefficent upper bounds.
    std::fill(p.row_coefficient_upper_bounds.begin(), p.row_coefficient_upper_bounds.end(), 1);
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = 10 * instance.item(0).profit;
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));
    return p;
}

std::vector<ColIdx> PricingSolver::initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns)
{
    std::fill(fixed_items_.begin(), fixed_items_.end(), -1);
    std::fill(fixed_knapsacks_.begin(), fixed_knapsacks_.end(), -1);
    for (auto p: fixed_columns) {
        const Column& column = columns[p.first];
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
            RowIdx row_index = column.row_indices[row_pos];
            Value row_coefficient = column.row_coefficients[row_pos];
            if (row_coefficient < 0.5)
                continue;
            if (row_index < instance_.number_of_knapsacks()) {
                fixed_knapsacks_[row_index] = 1;
            } else {
                fixed_items_[row_index - instance_.number_of_knapsacks()] = 1;
            }
        }
    }
    return {};
}

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    KnapsackId m = instance_.number_of_knapsacks();
    ItemId n = instance_.number_of_items();
    std::vector<Column> columns;
    knapsacksolver::Profit mult = 10000;
    for (KnapsackId i = 0; i < m; ++i) {
        if (fixed_knapsacks_[i] == 1)
            continue;
        // Build subproblem instance.
        knapsacksolver::Instance instance_kp;
        instance_kp.set_capacity(instance_.capacity(i));
        kp2mkp_.clear();
        for (ItemId j = 0; j < n; ++j) {
            if (fixed_items_[j] == 1)
                continue;
            knapsacksolver::Profit profit = std::floor(mult * instance_.profit(j))
                - std::ceil(mult * duals[m + j]);
            if (profit <= 0 || instance_.weight(j) > instance_.capacity(i))
                continue;
            instance_kp.add_item(instance_.weight(j), profit);
            kp2mkp_.push_back(j);
        }

        // Solve subproblem instance.
        auto output_kp = knapsacksolver::minknap(instance_kp);

        // Retrieve column.
        Column column;
        column.row_indices.push_back(i);
        column.row_coefficients.push_back(1);
        for (knapsacksolver::ItemIdx j = 0; j < instance_kp.number_of_items(); ++j) {
            if (output_kp.solution.contains_idx(j)) {
                column.row_indices.push_back(m + kp2mkp_[j]);
                column.row_coefficients.push_back(1);
                column.objective_coefficient += instance_.profit(kp2mkp_[j]);
            }
        }
        columns.push_back(column);
    }
    return columns;
}

}

}

