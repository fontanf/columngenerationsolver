/**
 * Star Observation Scheduling Problem.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/starobservationscheduling.hpp
 *
 * The linear programming formulation of the problem based on Dantzig–Wolfe
 * decomposition is written as follows:
 *
 * Variables:
 * - yᵢᵏ ∈ {0, 1} representing a feasible schedule of night i.
 *   yᵢᵏ = 1 iff the corresponding schedule is selected for night i.
 *   xⱼᵢᵏ = 1 iff yᵢᵏ contains target j, otherwise 0.
 *
 * Program:
 *
 * max ∑ᵢ ∑ₖ (∑ⱼ wⱼ xⱼᵢᵏ) yᵢᵏ
 *                                      Note that (∑ⱼ wⱼ xⱼᵢᵏ) is a constant.
 *
 * 0 <= ∑ₖ yᵢᵏ <= 1        for all nights i
 *                         (not more than 1 schedule selected for each night)
 *                                                         Dual variables: uᵢ
 * 0 <= ∑ₖ xⱼᵢᵏ yᵢᵏ <= 1   for all targets j
 *                                        (each target selected at most once)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of positive reduced cost.
 * The reduced cost of a variable yᵢᵏ is given by:
 * rc(yᵢᵏ) = ∑ⱼ wⱼ xⱼᵢᵏ - uᵢ - ∑ⱼ xⱼᵢᵏ vⱼ
 *         = ∑ⱼ (wⱼ - vⱼ) xⱼᵢᵏ - uᵢ
 *
 * Therefore, finding a variable of maximum reduced cost reduces to solving m
 * Single Night Star Observation Scheduling Problems with targets with profit
 * (wⱼ - vⱼ).
 *
 */

#pragma once

#include "columngenerationsolver/commons.hpp"

#include "examples/pricingsolver/singlenightstarobservationscheduling.hpp"

#include "orproblems/starobservationscheduling.hpp"

namespace columngenerationsolver
{

namespace starobservationscheduling
{

using namespace orproblems::starobservationscheduling;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        fixed_targets_(instance.number_of_targets()),
        fixed_nights_(instance.number_of_nights())
    {  }

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<int8_t> fixed_targets_;
    std::vector<int8_t> fixed_nights_;

    std::vector<TargetId> snsosp2sosp_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    NightId m = instance.number_of_nights();
    TargetId n = instance.number_of_targets();
    columngenerationsolver::Parameters p(m + n);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Max;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row lower bounds.
    std::fill(p.row_lower_bounds.begin(), p.row_lower_bounds.begin() + m, 0);
    std::fill(p.row_lower_bounds.begin() + m, p.row_lower_bounds.end(), 0);
    // Row upper bounds.
    std::fill(p.row_upper_bounds.begin(), p.row_upper_bounds.begin() + m, 1);
    std::fill(p.row_upper_bounds.begin() + m, p.row_upper_bounds.end(), 1);
    // Row coefficent lower bounds.
    std::fill(p.row_coefficient_lower_bounds.begin(), p.row_coefficient_lower_bounds.end(), 0);
    // Row coefficent upper bounds.
    std::fill(p.row_coefficient_upper_bounds.begin(), p.row_coefficient_upper_bounds.end(), 1);
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = -instance.total_profit();
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));
    return p;
}

std::vector<ColIdx> PricingSolver::initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns)
{
    std::fill(fixed_targets_.begin(), fixed_targets_.end(), -1);
    std::fill(fixed_nights_.begin(), fixed_nights_.end(), -1);
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
            if (row_index < instance_.number_of_nights()) {
                fixed_nights_[row_index] = 1;
            } else {
                fixed_targets_[row_index - instance_.number_of_nights()] = 1;
            }
        }
    }
    return {};
}

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    NightId m = instance_.number_of_nights();
    std::vector<Column> columns;
    singlenightstarobservationschedulingsolver::Profit mult = 10000;
    for (NightId i = 0; i < m; ++i) {
        if (fixed_nights_[i] == 1)
            continue;
        // Build subproblem instance.
        singlenightstarobservationschedulingsolver::Instance instance_snsosp;
        snsosp2sosp_.clear();
        for (TargetId j_pos = 0; j_pos < instance_.number_of_observables(i); ++j_pos) {
            auto o = instance_.observable(i, j_pos);
            if (fixed_targets_[o.j] == 1)
                continue;
            singlenightstarobservationschedulingsolver::Target target;
            target.profit
                = std::floor(mult * instance_.profit(o.j))
                - std::ceil(mult * duals[m + o.j]);
            if (target.profit <= 0)
                continue;
            target.r = o.r;
            target.d = o.d;
            target.p = o.p;
            instance_snsosp.add_target(target);
            snsosp2sosp_.push_back(o.j);
        }

        // Solve subproblem instance.
        auto output_snsosp = singlenightstarobservationschedulingsolver::dynamicprogramming(instance_snsosp);

        // Retrieve column.
        Column column;
        column.row_indices.push_back(i);
        column.row_coefficients.push_back(1);
        for (const auto& o: output_snsosp.observations()) {
            column.row_indices.push_back(m + snsosp2sosp_[o.j]);
            column.row_coefficients.push_back(1);
            column.objective_coefficient += instance_.profit(snsosp2sosp_[o.j]);
        }
        columns.push_back(column);
    }
    return columns;
}

}

}

