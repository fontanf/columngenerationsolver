#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct GreedyOptionalParameters: Parameters
{
    /** Parameters for the column generation sub-problem. */
    ColumnGenerationOptionalParameters column_generation_parameters;
};

struct GreedyOutput: Output
{
    /** Constructor. */
    GreedyOutput(const Model& model):
        Output(model) { }


    Counter number_of_nodes = 0;
};

const GreedyOutput greedy(
        const Model& model,
        const GreedyOptionalParameters& optional_parameters = {});

}
