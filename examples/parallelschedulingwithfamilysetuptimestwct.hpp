#pragma once

/**
 * Identical parallel machine scheduling problem with family setup times, Total
 * weighted completion time.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/parallelschedulingwithfamilysetuptimestwct.hpp
 *
 * The linear programming formulation of the problem based on Dantzig–Wolfe
 * decomposition is written as follows:
 *
 * Variables:
 * - yᵏ ∈ {0, 1} representing a feasible schedule for a machine.
 *   yᵏ = 1 iff the corresponding schedule is selected.
 *   cᵏ the total weighted completion time of schedule yᵏ.
 *   xⱼᵏ = 1 iff job j is scheduled in schedule yᵏ.
 *
 * Program:
 *
 * min ∑ₖ cᵏ yᵏ
 *
 * 0 <= ∑ₖ yᵏ <= m
 *                                                 (not more then m machines)
 *                                                           Dual variable: u
 * 1 <= ∑ₖ xⱼᵏ yᵏ <= 1     for all job j
 *                                       (each job is scheduled exactly once)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of negative reduced cost.
 * The reduced cost of a variable yᵏ is given by:
 * rc(yᵏ) = cᵏ - u - ∑ⱼ xⱼᵏ vⱼ
 *
 * Therefore, finding a variable of minimum reduced cost reduces to solving a
 * Single machine order acceptance and scheduling problem with family setup
 * times, Total weighted completion time.
 *
 */

#include "columngenerationsolver/commons.hpp"

#include "examples/pricingsolver/oaschedulingwithfamilysetuptimestwct.hpp"

#include "orproblems/parallelschedulingwithfamilysetuptimestwct.hpp"

#include "treesearchsolver/algorithms/iterative_beam_search.hpp"

#include "optimizationtools/utils.hpp"

namespace columngenerationsolver
{

namespace parallelschedulingwithfamilysetuptimestwct
{

using namespace orproblems::parallelschedulingwithfamilysetuptimestwct;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        scheduled_jobs_(instance.job_number(), 0)
    { }

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<int8_t> scheduled_jobs_;

    std::vector<JobId> smoas2pms_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    JobId n = instance.job_number();
    MachineId m = instance.machine_number();
    columngenerationsolver::Parameters p(n + 1);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Min;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row bounds.
    p.row_lower_bounds[0] = m;
    p.row_upper_bounds[0] = m;
    p.row_coefficient_lower_bounds[0] = 1;
    p.row_coefficient_upper_bounds[0] = 1;
    for (JobId j = 0; j < n; ++j) {
        p.row_lower_bounds[j + 1] = 1;
        p.row_upper_bounds[j + 1] = 1;
        p.row_coefficient_lower_bounds[j + 1] = 0;
        p.row_coefficient_upper_bounds[j + 1] = 1;
    }
    // Dummy column objective coefficient.
    Time t_max = 0;
    Weight w_max = 0;
    for (JobId j = 0; j < n; ++j) {
        t_max += instance.job(j).processing_time
            + instance.family(instance.job(j).family).setup_time;
        w_max = std::max(w_max, instance.job(j).weight);
    }
    p.dummy_column_objective_coefficient = w_max * t_max;
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));
    return p;
}

std::vector<ColIdx> PricingSolver::initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns)
{
    std::fill(scheduled_jobs_.begin(), scheduled_jobs_.end(), 0);
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
            if (row_index == 0)
                continue;
            scheduled_jobs_[row_index - 1] = 1;
        }
    }
    return {};
}

struct ColumnExtra
{
    std::vector<JobId> schedule;
};

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    JobId n = instance_.job_number();

    // Build subproblem instance.
    smoas2pms_.clear();
    for (JobId j = 0; j < n; ++j) {
        if (scheduled_jobs_[j] == 1)
            continue;
        smoas2pms_.push_back(j);
    }
    JobId n_smoas = smoas2pms_.size();
    if (n_smoas == 0)
        return {};
    oaschedulingwithfamilysetuptimestwct::Instance instance_smoas(instance_.family_number());
    for (FamilyId k = 0; k < instance_.family_number(); ++k)
        instance_smoas.set_setup_time(k, instance_.family(k).setup_time);
    for (JobId j_smoas = 0; j_smoas < n_smoas; ++j_smoas) {
        JobId j = smoas2pms_[j_smoas];
        instance_smoas.add_job(
                instance_.job(j).processing_time,
                instance_.job(j).weight,
                instance_.job(j).family,
                duals[j + 1]);
    }
    //std::cout << instance_smoas << std::endl;

    // Solve subproblem instance.
    oaschedulingwithfamilysetuptimestwct::BranchingScheme branching_scheme(instance_smoas);
    treesearchsolver::IterativeBeamSearchOptionalParameters parameters_smoas;
    parameters_smoas.solution_pool_size_max = 100;
    parameters_smoas.queue_size_min = 512;
    parameters_smoas.queue_size_max = 512;
    //parameters_smoas.info.set_verbose(true);
    auto output_smoas = treesearchsolver::iterativebeamsearch(
            branching_scheme, parameters_smoas);

    // Retrieve column.
    std::vector<Column> columns;
    JobId i = 0;
    for (const std::shared_ptr<oaschedulingwithfamilysetuptimestwct::BranchingScheme::Node>& node:
            output_smoas.solution_pool.solutions()) {
        if (i > 2 * n_smoas)
            break;
        std::vector<JobId> solution;
        for (auto node_tmp = node; node_tmp->father != nullptr; node_tmp = node_tmp->father)
            solution.push_back(smoas2pms_[node_tmp->j]);
        std::reverse(solution.begin(), solution.end());
        i += solution.size();

        Column column;
        column.objective_coefficient = node->total_weighted_completion_time;
        column.row_indices.push_back(0);
        column.row_coefficients.push_back(1);
        for (JobId j: solution) {
            column.row_indices.push_back(j + 1);
            column.row_coefficients.push_back(1);
        }
        ColumnExtra extra {solution};
        column.extra = std::shared_ptr<void>(new ColumnExtra(extra));
        columns.push_back(column);
    }
    return columns;
}

}

}

