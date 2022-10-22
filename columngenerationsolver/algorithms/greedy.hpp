#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct GreedyOptionalParameters
{
    /** Parameters for the column generation sub-problem. */
    ColumnGenerationOptionalParameters column_generation_parameters;

    /** Info structure. */
    optimizationtools::Info info = optimizationtools::Info();
};

struct GreedyOutput
{
    std::vector<std::pair<Column, Value>> solution;
    Value solution_value = 0;
    Value bound;
    double time_lpsolve = 0.0;
    double time_pricing = 0.0;
    Counter total_number_of_columns = 0;
    Counter number_of_added_columns = 0;
};

GreedyOutput greedy(
        Parameters& parameters,
        GreedyOptionalParameters optional_parameters = {});

}
