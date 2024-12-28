#pragma once

#include "columngenerationsolver/commons.hpp"
#include "columngenerationsolver/mixed_integer_linear_programming_solver.hpp"

namespace columngenerationsolver
{

struct RestrictedMasterOutput: Output
{
    /** Constructor. */
    RestrictedMasterOutput(const Model& model):
        Output(model) { }


    virtual int format_width() const override { return 31; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        //os
        //    << std::setw(width) << std::left << "Number of pricings: " << number_of_pricings << std::endl
        //    ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                //{"NumberOfPricings", number_of_pricings},
                });
        return json;
    }
};

using RestrictedMasterIterationCallback = std::function<void(const RestrictedMasterOutput&)>;

struct RestrictedMasterParameters: Parameters
{
    /** Linear programming solver. */
    MilpSolverName solver_name = MilpSolverName::CBC;

    /** Maximum number of iterations. */
    Counter maximum_number_of_nodes = -1;

    const Solution* initial_solution = nullptr;

    /** Callback function called at each column generation iteration. */
    RestrictedMasterIterationCallback iteration_callback = [](const Output&) { };


    virtual int format_width() const override { return 41; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Solver: " << solver_name << std::endl
            << std::setw(width) << std::left << "Maximum number of nodes: " << maximum_number_of_nodes << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                {"SolverName", solver_name},
                {"MaximumNumberOfNodes", maximum_number_of_nodes},
                });
        return json;
    }
};

const RestrictedMasterOutput restricted_master(
        const Model& model,
        const RestrictedMasterParameters& parameters = {});

}
