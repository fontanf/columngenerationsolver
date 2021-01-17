#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct GreedyOptionalParameters
{
    bool* end = NULL;
    ColumnGenerationOptionalParameters columngeneration_parameters;
    optimizationtools::Info info = optimizationtools::Info();
};

struct GreedyOutput
{
    std::vector<std::pair<Column, Value>> solution;
    Value solution_value = 0;
    Value bound;
    double time_lpsolve = 0.0;
    double time_pricing = 0.0;
    Counter total_column_number = 0;
    Counter added_column_number = 0;
};

inline GreedyOutput greedy(
        Parameters& parameters,
        GreedyOptionalParameters optional_parameters = {})
{
    VER(optional_parameters.info, "*** greedy ***" << std::endl);
    VER(optional_parameters.info, "---" << std::endl);
    VER(optional_parameters.info, "Linear programming solver:                " << optional_parameters.columngeneration_parameters.linear_programming_solver << std::endl);
    VER(optional_parameters.info, "Static Wentges smoothing parameter:       " << optional_parameters.columngeneration_parameters.static_wentges_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Static directional smoothing parameter:   " << optional_parameters.columngeneration_parameters.static_directional_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Self-adjusting Wentges smoothing:         " << optional_parameters.columngeneration_parameters.self_adjusting_wentges_smoothing << std::endl);
    VER(optional_parameters.info, "Automatic directional smoothing:          " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "Column generation iteration limit:        " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
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
        if (!optional_parameters.info.check_time())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;

        ColumnGenerationOptionalParameters columngeneration_parameters
            = optional_parameters.columngeneration_parameters;
        columngeneration_parameters.fixed_columns = &fixed_columns;
        columngeneration_parameters.end = optional_parameters.end;
        columngeneration_parameters.info.reset_time();
        columngeneration_parameters.info.set_timelimit(optional_parameters.info.remaining_time());
        //columngeneration_parameters.info.set_verbose(true);
        auto output_columngeneration = columngeneration(
                parameters,
                columngeneration_parameters);
        output.time_lpsolve += output_columngeneration.time_lpsolve;
        output.time_pricing += output_columngeneration.time_pricing;
        output.added_column_number += output_columngeneration.added_column_number;
        if (!optional_parameters.info.check_time())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
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
            Counter cg_it_limit = optional_parameters.columngeneration_parameters.iteration_limit;
            if (cg_it_limit == -1 || (output_columngeneration.iteration_number < cg_it_limit))
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

    output.total_column_number = parameters.columns.size();
    display_end(output, optional_parameters.info);
    return output;
}

}
