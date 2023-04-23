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

struct ColumnExtra
{
    NightId night_id;
    singlenightstarobservationschedulingsolver::Solution snsosp_solution;
    std::vector<ObservableId> snsosp2sosp;
};

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    std::vector<Column> columns;
    for (NightId night_id = 0;
            night_id < instance_.number_of_nights();
            ++night_id) {
        if (fixed_nights_[night_id] == 1)
            continue;

        // Build subproblem instance.
        singlenightstarobservationschedulingsolver::Instance snsosp_instance;
        snsosp2sosp_.clear();
        for (ObservableId observable_id = 0;
                observable_id < instance_.number_of_observables(night_id);
                ++observable_id) {
            const Observable& observable = instance_.observable(night_id, observable_id);
            if (fixed_targets_[observable.target_id] == 1)
                continue;
            singlenightstarobservationschedulingsolver::Profit profit
                = instance_.profit(observable.target_id)
                - duals[instance_.number_of_nights() + observable.target_id];
            if (profit <= 0)
                continue;
            snsosp_instance.add_target(
                    observable.release_date,
                    observable.deadline,
                    observable.observation_time,
                    profit);
            snsosp2sosp_.push_back(observable_id);
        }

        // Solve subproblem instance.
        auto snsosp_output = singlenightstarobservationschedulingsolver::dynamicprogramming(snsosp_instance);

        // Retrieve column.
        Column column;
        column.row_indices.push_back(night_id);
        column.row_coefficients.push_back(1);
        for (singlenightstarobservationschedulingsolver::TargetId snsosp_observation_pos = 0;
                snsosp_observation_pos < snsosp_output.number_of_observations();
                ++snsosp_observation_pos) {
            const auto& snsosp_observation = snsosp_output.observation(snsosp_observation_pos);
            ObservableId observable_id = snsosp2sosp_[snsosp_observation.target_id];
            const Observable& observable = instance_.observable(night_id, observable_id);
            column.row_indices.push_back(
                    instance_.number_of_nights() + observable.target_id);
            column.row_coefficients.push_back(1);
            column.objective_coefficient += instance_.profit(observable.target_id);
        }
        // Extra.
        ColumnExtra extra {night_id, snsosp_output, snsosp2sosp_};
        column.extra = std::shared_ptr<void>(new ColumnExtra(extra));
        columns.push_back(column);
    }
    return columns;
}

}

}

