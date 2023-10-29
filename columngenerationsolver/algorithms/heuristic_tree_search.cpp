#include "columngenerationsolver/algorithms/heuristic_tree_search.hpp"

using namespace columngenerationsolver;

HeuristicTreeSearchOutput columngenerationsolver::heuristic_tree_search(
        Parameters& parameters,
        HeuristicTreeSearchOptionalParameters optional_parameters)
{
    // Initial display.
    optional_parameters.info.os()
            << "======================================" << std::endl
            << "        ColumnGenerationSolver        " << std::endl
            << "======================================" << std::endl
            << std::endl
            << "Algorithm" << std::endl
            << "---------" << std::endl
            << "Heuristic Tree Search" << std::endl
            << std::endl
            << "Parameters" << std::endl
            << "----------" << std::endl
            << "Linear programming solver:               " << optional_parameters.column_generation_parameters.linear_programming_solver << std::endl
            << "Static Wentges smoothing parameter:      " << optional_parameters.column_generation_parameters.static_wentges_smoothing_parameter << std::endl
            << "Static directional smoothing parameter:  " << optional_parameters.column_generation_parameters.static_directional_smoothing_parameter << std::endl
            << "Self-adjusting Wentges smoothing:        " << optional_parameters.column_generation_parameters.self_adjusting_wentges_smoothing << std::endl
            << "Automatic directional smoothing:         " << optional_parameters.column_generation_parameters.automatic_directional_smoothing << std::endl
            << std::endl;

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

        // Clean column pool?
        //parameters.columns.clear();
        //for (const auto& p: output.solution)
        //    parameters.columns.push_back(p.first);
        LimitedDiscrepancySearchOptionalParameters lds_parameters;
        lds_parameters.info.set_time_limit(optional_parameters.info.remaining_time());
        lds_parameters.column_generation_parameters
            = optional_parameters.column_generation_parameters;
        lds_parameters.column_generation_parameters.maximum_number_of_iterations
            = output.maximum_number_of_iterations;
        lds_parameters.heuristictreesearch_stop = true;

        lds_parameters.new_bound_callback = [&parameters, &optional_parameters, &output](
                const columngenerationsolver::LimitedDiscrepancySearchOutput& lds_output)
        {
            if ((parameters.objective_sense == ObjectiveSense::Min
                        && output.bound + FFOT_TOL < lds_output.bound)
                    || (parameters.objective_sense == ObjectiveSense::Max
                        && output.bound - FFOT_TOL > lds_output.bound)) {
                output.bound = lds_output.bound;
                std::stringstream ss;
                ss << "it " << output.maximum_number_of_iterations;
                display(parameters, output.solution_value, output.bound, ss, optional_parameters.info);
                optional_parameters.new_bound_callback(output);
            }
            if (lds_output.solution.size() > 0) {
                // Update solution.
                if ((parameters.objective_sense == ObjectiveSense::Min
                            && output.solution_value - FFOT_TOL > lds_output.solution_value)
                        || (parameters.objective_sense == ObjectiveSense::Max
                            && output.solution_value + FFOT_TOL < lds_output.solution_value)) {
                    output.solution = lds_output.solution;
                    output.solution_value = lds_output.solution_value;
                    output.solution_iteration = output.maximum_number_of_iterations;
                    output.solution_node = lds_output.number_of_nodes;
                    std::stringstream ss;
                    ss << "it " << output.solution_iteration << " node " << output.solution_node;
                    display(parameters, output.solution_value, output.bound, ss, optional_parameters.info);
                    optional_parameters.new_bound_callback(output);
                }
            }
        };

        auto lds_output = limited_discrepancy_search(parameters, lds_parameters);

    }

    output.total_number_of_columns = parameters.columns.size();
    display_end(output, optional_parameters.info);
    return output;
}
