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
    Counter total_number_of_columns = 0;
    Counter number_of_added_columns = 0;
    Counter maximum_number_of_iterations = 0;
};

typedef std::function<void(const HeuristicTreeSearchOutput&)> HeuristicTreeSearchCallback;

struct HeuristicTreeSearchOptionalParameters
{
    HeuristicTreeSearchCallback new_bound_callback
        = [](const HeuristicTreeSearchOutput& o) { (void)o; };
    Counter number_of_threads = 3;
    double growth_rate = 1.5;
    bool* end = NULL;
    ColumnGenerationOptionalParameters columngeneration_parameters;
    optimizationtools::Info info = optimizationtools::Info();
};

inline HeuristicTreeSearchOutput heuristictreesearch(
        Parameters& parameters,
        HeuristicTreeSearchOptionalParameters optional_parameters = {})
{
    // Initial display.
    VER(optional_parameters.info,
               "======================================" << std::endl
            << "       Column Generation Solver       " << std::endl
            << "======================================" << std::endl
            << std::endl
            << "Algorithm" << std::endl
            << "---------" << std::endl
            << "Heuristic Tree Search" << std::endl
            << std::endl
            << "Parameters" << std::endl
            << "----------" << std::endl
            << "Linear programming solver:               " << optional_parameters.columngeneration_parameters.linear_programming_solver << std::endl
            << "Static Wentges smoothing parameter:      " << optional_parameters.columngeneration_parameters.static_wentges_smoothing_parameter << std::endl
            << "Static directional smoothing parameter:  " << optional_parameters.columngeneration_parameters.static_directional_smoothing_parameter << std::endl
            << "Self-adjusting Wentges smoothing:        " << optional_parameters.columngeneration_parameters.self_adjusting_wentges_smoothing << std::endl
            << "Automatic directional smoothing:         " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl
            << std::endl
       );

    HeuristicTreeSearchOutput output;
    output.solution_value = (parameters.objective_sense == ObjectiveSense::Min)?
        std::numeric_limits<Value>::infinity():
        -std::numeric_limits<Value>::infinity();
    output.bound = (parameters.objective_sense == ObjectiveSense::Min)?
        -std::numeric_limits<Value>::infinity():
        std::numeric_limits<Value>::infinity();
    display_initialize(optional_parameters.info);

    for (output.maximum_number_of_iterations = 0;;
            output.maximum_number_of_iterations *= optional_parameters.growth_rate) {
        if (output.maximum_number_of_iterations == (Counter)(output.maximum_number_of_iterations * optional_parameters.growth_rate))
            output.maximum_number_of_iterations++;

        if (optional_parameters.info.needs_to_end())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;

        // Clean column pool?
        //parameters.columns.clear();
        //for (const auto& p: output.solution)
        //    parameters.columns.push_back(p.first);
        LimitedDiscrepancySearchOptionalParameters parameters_limiteddiscrepancysearch;
        parameters_limiteddiscrepancysearch.info.set_time_limit(optional_parameters.info.remaining_time());
        parameters_limiteddiscrepancysearch.columngeneration_parameters
            = optional_parameters.columngeneration_parameters;
        parameters_limiteddiscrepancysearch.columngeneration_parameters.maximum_number_of_iterations
            = output.maximum_number_of_iterations;
        parameters_limiteddiscrepancysearch.heuristictreesearch_stop = true;

        parameters_limiteddiscrepancysearch.new_bound_callback = [&parameters, &optional_parameters, &output](
                const columngenerationsolver::LimitedDiscrepancySearchOutput& o)
        {
            if ((parameters.objective_sense == ObjectiveSense::Min
                        && output.bound + TOL < o.bound)
                    || (parameters.objective_sense == ObjectiveSense::Max
                        && output.bound - TOL > o.bound)) {
                output.bound = o.bound;
                std::stringstream ss;
                ss << "it " << output.maximum_number_of_iterations;
                display(parameters, output.solution_value, output.bound, ss, optional_parameters.info);
                optional_parameters.new_bound_callback(output);
            }
            if (o.solution.size() > 0) {
                // Update solution.
                if ((parameters.objective_sense == ObjectiveSense::Min
                            && output.solution_value - TOL > o.solution_value)
                        || (parameters.objective_sense == ObjectiveSense::Max
                            && output.solution_value + TOL < o.solution_value)) {
                    output.solution = o.solution;
                    output.solution_value = o.solution_value;
                    output.solution_iteration = output.maximum_number_of_iterations;
                    output.solution_node = o.number_of_nodes;
                    std::stringstream ss;
                    ss << "it " << output.solution_iteration << " node " << output.solution_node;
                    display(parameters, output.solution_value, output.bound, ss, optional_parameters.info);
                    optional_parameters.new_bound_callback(output);
                }
            }
        };

        auto output_limiteddiscrepancysearch = limiteddiscrepancysearch(parameters, parameters_limiteddiscrepancysearch);

    }

    output.total_number_of_columns = parameters.columns.size();
    display_end(output, optional_parameters.info);
    return output;
}

}
