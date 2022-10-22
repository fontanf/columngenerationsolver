#include "columngenerationsolver/algorithms/greedy.hpp"

using namespace columngenerationsolver;

GreedyOutput columngenerationsolver::greedy(
        Parameters& parameters,
        GreedyOptionalParameters optional_parameters)
{
    // Initial display.
    optional_parameters.info.os()
            << "======================================" << std::endl
            << "       Column Generation Solver       " << std::endl
            << "======================================" << std::endl
            << std::endl
            << "Algorithm" << std::endl
            << "---------" << std::endl
            << "Greedy" << std::endl
            << std::endl
            << "Parameters" << std::endl
            << "----------" << std::endl
            << "Linear programming solver:               " << optional_parameters.column_generation_parameters.linear_programming_solver << std::endl
            << "Static Wentges smoothing parameter:      " << optional_parameters.column_generation_parameters.static_wentges_smoothing_parameter << std::endl
            << "Static directional smoothing parameter:  " << optional_parameters.column_generation_parameters.static_directional_smoothing_parameter << std::endl
            << "Self-adjusting Wentges smoothing:        " << optional_parameters.column_generation_parameters.self_adjusting_wentges_smoothing << std::endl
            << "Automatic directional smoothing:         " << optional_parameters.column_generation_parameters.automatic_directional_smoothing << std::endl;

    GreedyOutput output;
    output.solution_value = (parameters.objective_sense == ObjectiveSense::Min)?
        std::numeric_limits<Value>::infinity():
        -std::numeric_limits<Value>::infinity();
    output.bound = (parameters.objective_sense == ObjectiveSense::Min)?
        -std::numeric_limits<Value>::infinity():
        std::numeric_limits<Value>::infinity();
    std::vector<std::pair<ColIdx, Value>> fixed_columns;

    for (;;) {
        // Check time
        if (optional_parameters.info.needs_to_end())
            break;

        ColumnGenerationOptionalParameters column_generation_parameters
            = optional_parameters.column_generation_parameters;
        column_generation_parameters.fixed_columns = &fixed_columns;
        column_generation_parameters.info = optimizationtools::Info(optional_parameters.info, false, "");
        //column_generation_parameters.info.set_verbose(true);
        auto output_columngeneration = column_generation(
                parameters,
                column_generation_parameters);
        output.time_lpsolve += output_columngeneration.time_lpsolve;
        output.time_pricing += output_columngeneration.time_pricing;
        output.number_of_added_columns += output_columngeneration.number_of_added_columns;
        if (optional_parameters.info.needs_to_end())
            break;
        if (output_columngeneration.solution.size() == 0)
            break;

        ColIdx col_best = -1;
        Value val_best = -1;
        Value diff_best = -1;
        Value bp_best = -1;
        for (auto p: output_columngeneration.solution) {
            ColIdx col = p.first;
            Value val = p.second;
            Value bp = parameters.columns[col].branching_priority;
            if (floor(val) != 0) {
                if (col_best == -1
                        || bp_best < bp
                        || (bp_best == bp && diff_best > val - floor(val))) {
                    col_best = col;
                    val_best = floor(val);
                    diff_best = val - floor(val);
                    bp_best = bp;
                }
            }
            if (ceil(val) != 0) {
                if (col_best == -1
                        || bp_best < bp
                        || (bp_best == bp && diff_best > ceil(val) - val)) {
                    col_best = col;
                    val_best = ceil(val);
                    diff_best = ceil(val) - val;
                    bp_best = bp;
                }
            }
        }
        if (col_best == -1)
            break;

        // Update bound.
        if (fixed_columns.size() == 0) {
            Counter cg_it_limit = optional_parameters.column_generation_parameters.maximum_number_of_iterations;
            if (cg_it_limit == -1 || (output_columngeneration.number_of_iterations < cg_it_limit))
                output.bound = output_columngeneration.solution_value;
        }
        // Update fixed columns.
        fixed_columns.push_back({col_best, val_best});
        // Update solution.
        if (is_feasible(parameters, fixed_columns)) {
            if (output.solution.size() == 0) {
                output.solution = to_solution(parameters, fixed_columns);
                output.solution_value = compute_value(parameters, fixed_columns);
            } else {
                Value solution_value = compute_value(parameters, fixed_columns);
                if (parameters.objective_sense == ObjectiveSense::Min
                        && output.solution_value > solution_value) {
                    output.solution = to_solution(parameters, fixed_columns);
                    output.solution_value = solution_value;
                }
                if (parameters.objective_sense == ObjectiveSense::Max
                        && output.solution_value < solution_value) {
                    output.solution = to_solution(parameters, fixed_columns);
                    output.solution_value = solution_value;
                }
            }
        }
    }

    output.total_number_of_columns = parameters.columns.size();
    display_end(output, optional_parameters.info);
    return output;
}
