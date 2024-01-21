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

    std::vector<std::pair<std::shared_ptr<const Column>, Value>> fixed_columns;

    for (output.number_of_nodes = 0;; ++ output.number_of_nodes) {

        // Check end.
        if (parameters.timer.needs_to_end())
            break;

        // Solve relaxation.
        ColumnGenerationParameters column_generation_parameters
            = parameters.column_generation_parameters;
        if (fixed_columns.empty()) {
            algorithm_formatter.print_column_generation_header();
            column_generation_parameters.iteration_callback = [&algorithm_formatter](
                    const ColumnGenerationOutput& cg_output)
            {
                algorithm_formatter.print_column_generation_iteration(
                        cg_output.number_of_column_generation_iterations,
                        cg_output.relaxation_solution.objective_value(),
                        cg_output.columns.size());
            };
        }
        column_generation_parameters.initial_columns.insert(
                column_generation_parameters.initial_columns.end(),
                output.columns.begin(),
                output.columns.end());
        column_generation_parameters.fixed_columns = &fixed_columns;
        column_generation_parameters.timer = parameters.timer;
        column_generation_parameters.verbosity_level = 0;

        // Solve.
        auto cg_output = column_generation(
                model,
                column_generation_parameters);

        // Update output statistics.
        output.time_lpsolve += cg_output.time_lpsolve;
        output.time_pricing += cg_output.time_pricing;
        output.number_of_column_generation_iterations += cg_output.number_of_column_generation_iterations;
        output.columns.insert(
                output.columns.end(),
                cg_output.columns.begin(),
                cg_output.columns.end());

        if (fixed_columns.empty()) {
            algorithm_formatter.print_header();
        }

        if (parameters.timer.needs_to_end())
            break;
        if (cg_output.relaxation_solution.columns().size() == 0)
            break;

        // Find column to branch on.
        std::shared_ptr<const Column> column_best = nullptr;
        Value value_best = -1;
        Value diff_best = -1;
        for (auto p: cg_output.relaxation_solution.columns()) {
            const std::shared_ptr<const Column>& column = p.first;
            Value value = p.second;
            if (floor(value) != 0) {
                if (column_best == nullptr
                        || column_best->branching_priority < column->branching_priority
                        || (column_best->branching_priority == column->branching_priority
                            && diff_best > value - floor(value))) {
                    column_best = column;
                    value_best = floor(value);
                    diff_best = value - floor(value);
                }
            }
            if (ceil(value) != 0) {
                if (column_best == nullptr
                        || column_best->branching_priority < column->branching_priority
                        || (column_best->branching_priority == column->branching_priority
                            && diff_best > ceil(value) - value)) {
                    column_best = column;
                    value_best = ceil(value);
                    diff_best = ceil(value) - value;
                }
            }
        }
        if (column_best == nullptr)
            break;

        // Update bound.
        if (fixed_columns.size() == 0) {
            Counter cg_it_limit = parameters.column_generation_parameters.maximum_number_of_iterations;
            if (cg_it_limit == -1
                    || (cg_output.number_of_column_generation_iterations < cg_it_limit)) {
                algorithm_formatter.update_bound(
                        cg_output.relaxation_solution.objective_value());
            }
        }

        // Update fixed columns.
        fixed_columns.push_back({column_best, value_best});

        // Update solution.
        SolutionBuilder solution_builder;
        solution_builder.set_model(model);
        for (const auto& p: fixed_columns)
            solution_builder.add_column(p.first, p.second);
        Solution solution = solution_builder.build();
        algorithm_formatter.update_solution(solution);
        algorithm_formatter.print("ndoe " + std::to_string(output.number_of_nodes));
    }

    algorithm_formatter.end();
    return output;
}
