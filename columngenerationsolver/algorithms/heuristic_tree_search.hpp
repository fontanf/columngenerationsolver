#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"
#include "columngenerationsolver/algorithms/greedy.hpp"
#include "columngenerationsolver/algorithms/limited_discrepancy_search.hpp"

namespace columngenerationsolver
{

struct HeuristicTreeSearchOptionalParameters: Parameters
{
    /** Growth rate. */
    double growth_rate = 1.5;

    /** Parameters for the column generation sub-problem. */
    ColumnGenerationOptionalParameters column_generation_parameters;
};

struct HeuristicTreeSearchOutput: Output
{
    /** Constructor. */
    HeuristicTreeSearchOutput(const Model& model):
        Output(model) { }


    Counter solution_iteration;

    Counter solution_node;

    Counter maximum_number_of_iterations = 0;
};

const HeuristicTreeSearchOutput heuristic_tree_search(
        const Model& model,
        const HeuristicTreeSearchOptionalParameters& optional_parameters = {});

}
