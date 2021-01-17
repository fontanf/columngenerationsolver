#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"
#include "columngenerationsolver/algorithms/greedy.hpp"

namespace columngenerationsolver
{

struct HeuristicTreeSearchOutput
{
    std::vector<std::pair<Column, Value>> solution;
    Value solution_value = 0;
    Value bound;
    Counter total_column_number = 0;
    Counter added_column_number = 0;
    Counter iteration_number_max = 0;
};

typedef std::function<void(const HeuristicTreeSearchOutput&)> HeuristicTreeSearchCallback;

struct HeuristicTreeSearchOptionalParameters
{
    HeuristicTreeSearchCallback new_bound_callback
        = [](const HeuristicTreeSearchOutput& o) { (void)o; };
    Counter thread_number = 3;
    double growth_rate = 1.5;
    bool* end = NULL;
    ColumnGenerationOptionalParameters columngeneration_parameters;
    optimizationtools::Info info = optimizationtools::Info();
};

inline HeuristicTreeSearchOutput heuristictreesearch(
        Parameters& parameters,
        HeuristicTreeSearchOptionalParameters optional_parameters = {})
{
    VER(optional_parameters.info, "*** heuristictreesearch ***" << std::endl);
    VER(optional_parameters.info, "---" << std::endl);
    VER(optional_parameters.info, "Linear programming solver:                " << optional_parameters.columngeneration_parameters.linear_programming_solver << std::endl);
    VER(optional_parameters.info, "Static Wentges smoothing parameter:       " << optional_parameters.columngeneration_parameters.static_wentges_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Static directional smoothing parameter:   " << optional_parameters.columngeneration_parameters.static_directional_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Self-adjusting Wentges smoothing:         " << optional_parameters.columngeneration_parameters.self_adjusting_wentges_smoothing << std::endl);
    VER(optional_parameters.info, "Automatic directional smoothing:          " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "---" << std::endl);

    HeuristicTreeSearchOutput output;
    output.solution_value = (parameters.objective_sense == ObjectiveSense::Min)?
        std::numeric_limits<Value>::infinity():
        -std::numeric_limits<Value>::infinity();
    output.bound = (parameters.objective_sense == ObjectiveSense::Min)?
        -std::numeric_limits<Value>::infinity():
        std::numeric_limits<Value>::infinity();
    display_initialize(parameters, optional_parameters.info);

    for (output.iteration_number_max = 0;;
            output.iteration_number_max *= optional_parameters.growth_rate) {
        if (output.iteration_number_max == (Counter)(output.iteration_number_max * optional_parameters.growth_rate))
            output.iteration_number_max++;

        // Clean column pool?
        //parameters.columns.clear();
        //for (const auto& p: output.solution)
        //    parameters.columns.push_back(p.first);
        GreedyOptionalParameters parameters_greedy;
        parameters_greedy.columngeneration_parameters
            = optional_parameters.columngeneration_parameters;
        parameters_greedy.columngeneration_parameters.iteration_limit
            = output.iteration_number_max;
        auto output_greedy = greedy(parameters, parameters_greedy);

        // Update solution.
        if ((parameters.objective_sense == ObjectiveSense::Min
                    && output.solution_value > output_greedy.solution_value)
                || (parameters.objective_sense == ObjectiveSense::Max
                        && output.solution_value < output_greedy.solution_value)) {
            output.solution = output_greedy.solution;
            output.solution_value = output_greedy.solution_value;
            std::stringstream ss;
            ss << "it " << output.iteration_number_max;
            display(output.solution_value, output.bound, ss, optional_parameters.info);
            optional_parameters.new_bound_callback(output);
        }

    }

    output.total_column_number = parameters.columns.size();
    display_end(output, optional_parameters.info);
    return output;
}

}
