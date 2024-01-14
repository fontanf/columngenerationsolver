#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct LimitedDiscrepancySearchOptionalParameters: Parameters
{
    /** Maximum discrepancy. */
    Value discrepancy_limit = std::numeric_limits<Value>::infinity();

    /** Specific stop criteria for the Heuristic Tree Search algorithm. */
    bool heuristictreesearch_stop = false;

    bool continue_until_feasible = false;

    /** Parameters for the column generation sub-problem. */
    ColumnGenerationOptionalParameters column_generation_parameters;
};

struct LimitedDiscrepancySearchOutput: Output
{
    /** Constructor. */
    LimitedDiscrepancySearchOutput(const Model& model):
        Output(model) { }


    Value solution_discrepancy = -1;

    Counter number_of_nodes = 0;

    Counter maximum_depth = 0;
};

const LimitedDiscrepancySearchOutput limited_discrepancy_search(
        const Model& model,
        const LimitedDiscrepancySearchOptionalParameters& optional_parameters = {});

}
