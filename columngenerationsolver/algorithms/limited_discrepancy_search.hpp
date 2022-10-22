#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct LimitedDiscrepancySearchOutput
{
    std::vector<std::pair<Column, Value>> solution;
    Value solution_value = 0;
    Value solution_discrepancy = -1;
    Value bound;
    Counter number_of_nodes = 0;
    Counter maximum_depth = 0;
    double time_lpsolve = 0.0;
    double time_pricing = 0.0;
    Counter total_number_of_columns = 0;
    Counter number_of_added_columns = 0;
};

typedef std::function<void(const LimitedDiscrepancySearchOutput&)> LimitedDiscrepancySearchCallback;

struct LimitedDiscrepancySearchOptionalParameters
{
    /** New bound callback. */
    LimitedDiscrepancySearchCallback new_bound_callback
        = [](const LimitedDiscrepancySearchOutput&) { };

    /** Maximum discrepancy. */
    Value discrepancy_limit = std::numeric_limits<Value>::infinity();

    /** Specific stop criteria for the Heuristic Tree Search algorithm. */
    bool heuristictreesearch_stop = false;

    bool continue_until_feasible = false;

    /** Parameters for the column generation sub-problem. */
    ColumnGenerationOptionalParameters column_generation_parameters;

    /** Info structure. */
    optimizationtools::Info info = optimizationtools::Info();
};

struct LimitedDiscrepancySearchNode
{
    std::shared_ptr<LimitedDiscrepancySearchNode> father = nullptr;
    ColIdx col = -1;
    Value value = 0;
    Value discrepancy = 0;
    Value value_sum = 1;
    ColIdx depth = 0;
};

LimitedDiscrepancySearchOutput limited_discrepancy_search(
        Parameters& parameters,
        LimitedDiscrepancySearchOptionalParameters optional_parameters = {});

}
