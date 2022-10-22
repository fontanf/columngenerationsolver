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

    // Stabilization parameters.
    Value static_wentges_smoothing_parameter = 0; // alpha
    bool self_adjusting_wentges_smoothing = false;
    Value static_directional_smoothing_parameter = 0.0; // beta
    bool automatic_directional_smoothing = false;

    /** Info structure. */
    optimizationtools::Info info = optimizationtools::Info();
};

struct ColumnGenerationOutput
{
    std::vector<std::pair<ColIdx, Value>> solution;
    Value solution_value;
    Counter number_of_iterations = 0;
    Counter number_of_added_columns = 0;
    Counter number_of_good_pricings = 0;
    Counter number_of_pricings = 0;
    Counter number_of_first_try_pricings = 0;
    Counter number_of_mispricings = 0;
    Counter number_of_no_stab_pricings = 0;
    double time_pricing = 0.0;
    double time_lpsolve = 0.0;
};

ColumnGenerationOutput column_generation(
        Parameters& parameters,
        ColumnGenerationOptionalParameters optional_parameters = {});

}
