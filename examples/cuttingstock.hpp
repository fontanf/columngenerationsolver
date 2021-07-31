#pragma once

/**
 * Cutting Stock Problem.
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

#include "columngenerationsolver/commons.hpp"

#include "orproblems/cuttingstock.hpp"

#include "knapsacksolver/algorithms/minknap.hpp"

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
    for (ItemTypeId j = 0; j < n; ++j) {
        p.row_lower_bounds[j] = instance.demand(j);
        p.row_upper_bounds[j] = instance.demand(j);
        p.row_coefficient_lower_bounds[j] = 0;
        p.row_coefficient_upper_bounds[j] = instance.demand(j);
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
    ItemTypeId n = instance_.number_of_item_types();
    knapsacksolver::Profit mult = 10000;

    // Build subproblem instance.
    knapsacksolver::Instance instance_kp;
    instance_kp.set_capacity(instance_.capacity());
    kp2csp_.clear();
    for (ItemTypeId j = 0; j < n; ++j) {
        knapsacksolver::Profit profit = std::floor(mult * duals[j]);
        if (profit <= 0)
            continue;
        for (Demand q = filled_demands_[j]; q < instance_.demand(j); ++q) {
            instance_kp.add_item(instance_.weight(j), profit);
            kp2csp_.push_back(j);
        }
    }

    // Solve subproblem instance.
    knapsacksolver::MinknapOptionalParameters parameters_kp;
    //parameters_kp.info.set_verbose(true);
    auto output_kp = knapsacksolver::minknap(instance_kp, parameters_kp);

    // Retrieve column.
    Column column;
    column.objective_coefficient = 1;
    std::vector<Demand> demands(instance_.number_of_item_types(), 0);
    for (knapsacksolver::ItemIdx j = 0; j < instance_kp.number_of_items(); ++j)
        if (output_kp.solution.contains_idx(j))
            demands[kp2csp_[j]]++;
    for (ItemTypeId j = 0; j < n; ++j) {
        if (demands[j] > 0) {
            column.row_indices.push_back(j);
            column.row_coefficients.push_back(demands[j]);
        }
    }
    return {column};
}

}

}
