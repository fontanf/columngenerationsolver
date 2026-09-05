#include "columngenerationsolver/algorithms/greedy.hpp"

#include "columngenerationsolver/algorithm_formatter.hpp"

using namespace columngenerationsolver;

const GreedyOutput columngenerationsolver::greedy(
        const Model& model,
        const GreedyParameters& parameters)
{
    // Initial display.
    GreedyOutput output(model);
    AlgorithmFormatter algorithm_formatter(
            model,
            parameters,
            output);
    algorithm_formatter.start("Greedy");

    std::vector<std::shared_ptr<const Column>> column_pool = parameters.column_pool;
    std::vector<std::shared_ptr<const Column>> initial_columns = parameters.initial_columns;
    std::vector<std::shared_ptr<const Cut>> cuts = parameters.initial_cuts;
    ColumnMap fixed_columns;

    for (output.number_of_nodes = 0;; ++ output.number_of_nodes) {

        // Check end.
        if (parameters.timer.needs_to_end())
            break;

        // Solve relaxation.
        ColumnGenerationParameters column_generation_parameters
            = parameters.column_generation_parameters;
        column_generation_parameters.timer = parameters.timer;
        column_generation_parameters.verbosity_level = 0;
        column_generation_parameters.internal_diving = node_activation(
                parameters.internal_diving, output.number_of_nodes == 0);
        column_generation_parameters.cutting_planes = node_activation(
                parameters.cutting_planes, output.number_of_nodes == 0);
        column_generation_parameters.rounding_heuristic = node_activation(
                parameters.rounding_heuristic, output.number_of_nodes == 0);
        column_generation_parameters.new_solution_callback = [&algorithm_formatter](
                const Output& cg_output)
        {
            algorithm_formatter.update_solution(cg_output.solution);
        };
        if (output.number_of_nodes == 0) {
            algorithm_formatter.print_column_generation_header();
            column_generation_parameters.iteration_callback = [&algorithm_formatter](
                    const ColumnGenerationOutput& cg_output)
            {
                algorithm_formatter.print_column_generation_iteration(
                        cg_output.number_of_column_generation_iterations,
                        cg_output.number_of_columns_in_linear_subproblem,
                        cg_output.relaxation_solution_value,
                        cg_output.bound);
            };
            column_generation_parameters.phase_callback = [&algorithm_formatter](
                    bool solve_feasibility)
            {
                algorithm_formatter.print_column_generation_phase_header(solve_feasibility);
            };
            column_generation_parameters.cutting_plane_callback = [&algorithm_formatter](
                    Counter cutting_plane_iteration)
            {
                algorithm_formatter.print_column_generation_cutting_plane_header(cutting_plane_iteration);
            };
            column_generation_parameters.new_bound_callback = [&algorithm_formatter](
                    const Output& cg_output)
            {
                algorithm_formatter.update_bound(cg_output.bound);
            };
        }
        column_generation_parameters.initial_columns.insert(
                column_generation_parameters.initial_columns.end(),
                initial_columns.begin(),
                initial_columns.end());
        column_generation_parameters.column_pool = column_pool;
        column_generation_parameters.initial_cuts = cuts;
        column_generation_parameters.fixed_columns = fixed_columns.columns();

        // Solve.
        auto cg_output = column_generation(
                model,
                column_generation_parameters);

        // Update output statistics.
        output.time_lpsolve += cg_output.time_lpsolve;
        output.time_pricing += cg_output.time_pricing;
        output.time_dual_pricing += cg_output.time_dual_pricing;
        output.time_column_pool_search += cg_output.time_column_pool_search;
        output.time_lp_construction += cg_output.time_lp_construction;
        output.time_dummy_free_verification += cg_output.time_dummy_free_verification;
        output.time_separation += cg_output.time_separation;
        output.time_rounding_heuristic += cg_output.time_rounding_heuristic;
        output.number_of_column_generation_iterations += cg_output.number_of_column_generation_iterations;
        output.columns.insert(
                output.columns.end(),
                cg_output.columns.begin(),
                cg_output.columns.end());
        column_pool.insert(
                column_pool.end(),
                cg_output.columns.begin(),
                cg_output.columns.end());
        cuts = cg_output.cuts;

        // Print header.
        if (output.number_of_nodes == 0)
            algorithm_formatter.print_greedy_header();

        // Check time.
        if (parameters.timer.needs_to_end())
            break;

        if (!cg_output.relaxation_solution_is_feasible)
            break;

        // 'new_bound_callback' above already streamed the bound as column
        // generation improved it; just record the final relaxation
        // solution here.
        if (output.number_of_nodes == 0) {
            output.relaxation_solution = cg_output.relaxation_solution;
            output.root_relaxation_solution = cg_output.relaxation_solution;
            output.root_cuts = cg_output.cuts;
        }

        // If the relaxation is (integer) feasible, save the solution and stop.
        if (cg_output.relaxation_solution.feasible()) {
            algorithm_formatter.update_solution(cg_output.relaxation_solution);
            algorithm_formatter.print_greedy_iteration(
                    output.number_of_nodes,
                    cg_output.relaxation_solution.objective_value());
            break;
        }

        // Try rounded solution.
        SolutionBuilder rounded_solution_builder;
        rounded_solution_builder.set_model(model);
        for (auto p: cg_output.relaxation_solution.columns()) {
            if (p.first->type == VariableType::Continuous) {
                rounded_solution_builder.add_column(p.first, p.second);
            } else {
                if (std::round(p.second) != 0)
                    rounded_solution_builder.add_column(p.first, std::round(p.second));
            }
        }
        Solution rounded_solution = rounded_solution_builder.build();
        if (rounded_solution.feasible())
            algorithm_formatter.update_solution(rounded_solution);
        algorithm_formatter.print_greedy_iteration(
                output.number_of_nodes,
                cg_output.relaxation_solution.objective_value());

        // Fix columns with value >= 1 to their floor value.
        bool fixed_found = false;
        for (auto p: cg_output.relaxation_solution.columns()) {
            const std::shared_ptr<const Column>& column = p.first;
            Value value = std::floor(p.second);
            if (value <= fixed_columns.get_column_value(column))
                continue;
            fixed_columns.set_column_value(column, value);
            fixed_found = true;
        }

        if (!fixed_found) {
            // Find column to branch on.
            std::shared_ptr<const Column> column_best = nullptr;
            Value value_best = -1;
            Value diff_best = -1;
            for (auto p: cg_output.relaxation_solution.columns()) {
                const std::shared_ptr<const Column>& column = p.first;
                Value value = p.second;

                // Don't branch on continuous variables.
                if (p.first->type == VariableType::Continuous)
                    continue;

                // Don't branch on a fixed column.
                if (value <= fixed_columns.get_column_value(column))
                    continue;

                // Don't fix a column to 0.
                if (ceil(value) == 0)
                    continue;

                if (column_best == nullptr
                        || column_best->branching_priority < column->branching_priority
                        || (column_best->branching_priority == column->branching_priority
                            && diff_best > ceil(value) - value)) {
                    column_best = column;
                    value_best = ceil(value);
                    diff_best = ceil(value) - value;
                }
            }
            if (column_best == nullptr)
                break;

            // Update fixed columns.
            fixed_columns.set_column_value(column_best, value_best);
        }

        // Update initial columns for the next node.
        initial_columns.clear();
        for (const auto& p: cg_output.relaxation_solution.columns())
            initial_columns.push_back(p.first);
    }

    algorithm_formatter.end();
    return output;
}
