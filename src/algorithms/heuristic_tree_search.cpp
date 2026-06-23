#include "columngenerationsolver/algorithms/heuristic_tree_search.hpp"

#include "columngenerationsolver/algorithm_formatter.hpp"
#include "columngenerationsolver/algorithms/limited_discrepancy_search.hpp"

using namespace columngenerationsolver;

const HeuristicTreeSearchOutput columngenerationsolver::heuristic_tree_search(
        const Model& model,
        const HeuristicTreeSearchParameters& parameters)
{
    // Initial display.
    HeuristicTreeSearchOutput output(model);
    AlgorithmFormatter algorithm_formatter(
            model,
            parameters,
            output);
    algorithm_formatter.start("Heuristic tree search");
    output.dummy_column_objective_coefficient = parameters.dummy_column_objective_coefficient;

    std::vector<std::shared_ptr<const Column>> column_pool;
    for (output.maximum_number_of_iterations = 0;;
            output.maximum_number_of_iterations *= parameters.growth_rate) {
        if (output.maximum_number_of_iterations == (Counter)(output.maximum_number_of_iterations * parameters.growth_rate))
            output.maximum_number_of_iterations++;

        // Check end.
        if (parameters.timer.needs_to_end())
            break;

        LimitedDiscrepancySearchParameters lds_parameters;
        lds_parameters.timer = parameters.timer;
        lds_parameters.dummy_column_objective_coefficient
            = output.dummy_column_objective_coefficient;
        lds_parameters.column_generation_parameters
            = parameters.column_generation_parameters;
        lds_parameters.column_generation_parameters.maximum_number_of_iterations
            = output.maximum_number_of_iterations;
        lds_parameters.automatic_stop = true;
        lds_parameters.column_pool = column_pool;

        lds_parameters.new_solution_callback = [&algorithm_formatter](
                const Output& callback_output)
        {
            const LimitedDiscrepancySearchOutput& lds_output = static_cast<const LimitedDiscrepancySearchOutput&>(callback_output);
            algorithm_formatter.update_solution(lds_output.solution);
            algorithm_formatter.update_bound(lds_output.bound);
        };

        auto lds_output = limited_discrepancy_search(model, lds_parameters);

        output.dummy_column_objective_coefficient
            = lds_output.dummy_column_objective_coefficient;
        column_pool.insert(
                column_pool.end(),
                lds_output.columns.begin(),
                lds_output.columns.end());

    }

    algorithm_formatter.end();
    return output;
}
