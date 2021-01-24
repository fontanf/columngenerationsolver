#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"
#include "columngenerationsolver/algorithms/greedy.hpp"
#include "columngenerationsolver/algorithms/limited_discrepancy_search.hpp"

namespace columngenerationsolver
{

struct HeuristicTreeSearchOutput
{
    std::vector<std::pair<Column, Value>> solution;
    Value solution_value = 0;
    Value bound;
    Counter solution_iteration;
    Counter solution_node;
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

        if (!optional_parameters.info.check_time())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;

        // Clean column pool?
        //parameters.columns.clear();
        //for (const auto& p: output.solution)
        //    parameters.columns.push_back(p.first);
        LimitedDiscrepancySearchOptionalParameters parameters_limiteddiscrepancysearch;
        parameters_limiteddiscrepancysearch.info.set_timelimit(optional_parameters.info.remaining_time());
        parameters_limiteddiscrepancysearch.columngeneration_parameters
            = optional_parameters.columngeneration_parameters;
        parameters_limiteddiscrepancysearch.columngeneration_parameters.iteration_limit
            = output.iteration_number_max;
        parameters_limiteddiscrepancysearch.heuristictreesearch_stop = true;

        parameters_limiteddiscrepancysearch.new_bound_callback = [&parameters, &optional_parameters, &output](
                const columngenerationsolver::LimitedDiscrepancySearchOutput& o)
        {
            if ((parameters.objective_sense == ObjectiveSense::Min
                        && output.bound < o.bound)
                    || (parameters.objective_sense == ObjectiveSense::Max
                        && output.bound > o.bound)) {
                output.bound = o.bound;
                std::stringstream ss;
                ss << "it " << output.iteration_number_max;
                display(output.solution_value, output.bound, ss, optional_parameters.info);
                optional_parameters.new_bound_callback(output);
            }
            if (o.solution.size() > 0) {
                // Update solution.
                if ((parameters.objective_sense == ObjectiveSense::Min
                            && output.solution_value > o.solution_value)
                        || (parameters.objective_sense == ObjectiveSense::Max
                            && output.solution_value < o.solution_value)) {
                    output.solution = o.solution;
                    output.solution_value = o.solution_value;
                    output.solution_iteration = output.iteration_number_max;
                    output.solution_node = o.node_number;
                    std::stringstream ss;
                    ss << "it " << output.solution_iteration << " node " << output.solution_node;
                    display(output.solution_value, output.bound, ss, optional_parameters.info);
                    optional_parameters.new_bound_callback(output);
                }
                //std::cout << "toto" << std::endl;
                optional_parameters.new_bound_callback(output);
            }
        };

        auto output_limiteddiscrepancysearch = limiteddiscrepancysearch(parameters, parameters_limiteddiscrepancysearch);

    }

    output.total_column_number = parameters.columns.size();
    display_end(output, optional_parameters.info);
    return output;
}

}
