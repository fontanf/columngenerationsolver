#pragma once

#include "columngenerationsolver/linear_programming_solver.hpp"

namespace columngenerationsolver
{

struct ColumnGenerationOptionalParameters
{
    /** Linear programming solver. */
    LinearProgrammingSolver linear_programming_solver = LinearProgrammingSolver::CLP;

    /** Pointer to a vector containing the columns fixed. */
    const std::vector<std::pair<ColIdx, Value>>* fixed_columns = NULL;

    /** Maximum number of iterations. */
    Counter maximum_number_of_iterations = -1;

    /*
     * Stabilization parameters
     */

    /** Static Wentges smoothing parameter (alpha). */
    Value static_wentges_smoothing_parameter = 0;

    /** Enable self-adjusting Wentges smoothing. */
    bool self_adjusting_wentges_smoothing = false;

    /** Static directional smoothing parameter (beta). */
    Value static_directional_smoothing_parameter = 0.0;

    /** Enable automatic directional smoothing. */
    bool automatic_directional_smoothing = false;

    /** Info structure. */
    optimizationtools::Info info = optimizationtools::Info();
};

struct ColumnGenerationOutput
{
    /**
     * Solution (fractional) found at the end of the column generation
     * procedure, given as a list of column id from the column pool and value.
     */
    std::vector<std::pair<ColIdx, Value>> solution;

    /** Value of the solution. */
    Value solution_value;

    /** Number of column generation iterations. */
    Counter number_of_iterations = 0;

    /** Number of columns added. */
    Counter number_of_added_columns = 0;

    /** Number of times the pricing algorithm has been called. */
    Counter number_of_pricings = 0;

    Counter number_of_first_try_pricings = 0;

    Counter number_of_mispricings = 0;

    Counter number_of_no_stab_pricings = 0;

    /** Time spent solving the pricing subproblems. */
    double time_pricing = 0.0;

    /** Time spent solving the LP subproblems. */
    double time_lpsolve = 0.0;
};

ColumnGenerationOutput column_generation(
        Parameters& parameters,
        ColumnGenerationOptionalParameters optional_parameters = {});

}
