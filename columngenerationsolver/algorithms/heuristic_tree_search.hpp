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

using HeuristicTreeSearchCallback = std::function<void(const HeuristicTreeSearchOutput&)>;

struct HeuristicTreeSearchOptionalParameters
{
    /** New bound callback. */
    HeuristicTreeSearchCallback new_bound_callback
        = [](const HeuristicTreeSearchOutput&) { };

    /** Growth rate. */
    double growth_rate = 1.5;

    /** Parameters for the column generation sub-problem. */
    ColumnGenerationOptionalParameters column_generation_parameters;

    /** Info structure. */
    optimizationtools::Info info = optimizationtools::Info();
};

HeuristicTreeSearchOutput heuristic_tree_search(
        Parameters& parameters,
        HeuristicTreeSearchOptionalParameters optional_parameters = {});

}
