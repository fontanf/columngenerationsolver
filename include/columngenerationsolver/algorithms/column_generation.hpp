#pragma once

#include "columngenerationsolver/linear_programming_solver.hpp"

namespace columngenerationsolver
{

struct ColumnGenerationOutput: Output
{
    /** Constructor. */
    ColumnGenerationOutput(const Model& model):
        Output(model) { }


    /** Value of the relaxation solution (with dummy columns). */
    double relaxation_solution_value = 0.0;

    /** Number of columns in the linear subproblem. */
    ColIdx number_of_columns_in_linear_subproblem = 0;

    /** Number of times the pricing algorithm has been called. */
    Counter number_of_pricings = 0;

    Counter number_of_first_try_pricings = 0;

    Counter number_of_mispricings = 0;

    Counter number_of_no_stab_pricings = 0;


    virtual int format_width() const override { return 31; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Number of pricings: " << number_of_pricings << std::endl
            << std::setw(width) << std::left << "Number of first-try pricings: " << number_of_first_try_pricings << std::endl
            << std::setw(width) << std::left << "Number of mispricings: " << number_of_mispricings << std::endl
            << std::setw(width) << std::left << "Number of no-stab pricings: " << number_of_no_stab_pricings << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                {"NumberOfPricings", number_of_pricings},
                {"NumberOfFirstTryPricings", number_of_first_try_pricings},
                {"NumberOfMispricings", number_of_mispricings},
                {"NumberOfNoStabPricings", number_of_no_stab_pricings},
                });
        return json;
    }
};

using ColumnGenerationIterationCallback = std::function<void(const ColumnGenerationOutput&)>;

struct ColumnGenerationParameters: Parameters
{
    /** Linear programming solver. */
    LinearProgrammingSolver linear_programming_solver = LinearProgrammingSolver::CLP;

    /** Maximum number of iterations. */
    Counter maximum_number_of_iterations = -1;

    /** Callback function called at each column generation iteration. */
    ColumnGenerationIterationCallback iteration_callback = [](const Output&) { };

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


    virtual int format_width() const override { return 41; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Linear programming solver: " << linear_programming_solver << std::endl
            << std::setw(width) << std::left << "Static Wentges smoothing parameter: " << static_wentges_smoothing_parameter << std::endl
            << std::setw(width) << std::left << "Static directional smoothing parameter: " << static_directional_smoothing_parameter << std::endl
            << std::setw(width) << std::left << "Self-adjusting Wentges smoothing: " << self_adjusting_wentges_smoothing << std::endl
            << std::setw(width) << std::left << "Automatic directional smoothing: " << automatic_directional_smoothing << std::endl
            << std::setw(width) << std::left << "Maximum number of iterations: " << maximum_number_of_iterations << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                {"LinearProgrammingSolver", linear_programming_solver},
                {"StaticWentgesSmoothingParameter", static_wentges_smoothing_parameter},
                {"StaticDirectionalSmoothingParameter", static_directional_smoothing_parameter},
                {"SelfAdjustingWentgesSmoothing", self_adjusting_wentges_smoothing},
                {"AutomaticDirectionalSmoothing", automatic_directional_smoothing},
                {"MaximumNumberOfIterations", maximum_number_of_iterations},
                });
        return json;
    }
};

const ColumnGenerationOutput column_generation(
        const Model& model,
        const ColumnGenerationParameters& parameters = {});

}
