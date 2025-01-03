#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct LimitedDiscrepancySearchParameters: Parameters
{
    /** Maximum discrepancy. */
    Value discrepancy_limit = std::numeric_limits<Value>::infinity();

    /** Specific stop criteria for the Heuristic Tree Search algorithm. */
    bool automatic_stop = false;

    bool continue_until_feasible = false;

    bool bound = false;

    /** Parameters for the column generation sub-problem. */
    ColumnGenerationParameters column_generation_parameters;


    virtual int format_width() const override { return 39; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Discrepancy limit: " << discrepancy_limit << std::endl
            << std::setw(width) << std::left << "Automatic stop: " << automatic_stop << std::endl
            << std::setw(width) << std::left << "Continue until feasible: " << continue_until_feasible << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                {"DiscrepancyLimit", discrepancy_limit},
                {"AutomaticStop", automatic_stop},
                {"ContinueUntilFeasible", continue_until_feasible},
                });
        return json;
    }
};

struct LimitedDiscrepancySearchOutput: Output
{
    /** Constructor. */
    LimitedDiscrepancySearchOutput(const Model& model):
        Output(model) { }


    Counter number_of_nodes = 0;

    Counter maximum_depth = 0;

    Value maximum_discrepancy = -1;


    virtual int format_width() const override { return 30; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Number of nodes: " << number_of_nodes << std::endl
            << std::setw(width) << std::left << "Maximum depth: " << maximum_depth << std::endl
            << std::setw(width) << std::left << "Maximum discrepancy: " << maximum_discrepancy << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                {"NumberOfNodes", number_of_nodes},
                {"MaximumDepth", maximum_depth},
                {"MaximumDiscrepancy", maximum_discrepancy},
                });
        return json;
    }
};

const LimitedDiscrepancySearchOutput limited_discrepancy_search(
        const Model& model,
        const LimitedDiscrepancySearchParameters& parameters = {});

}
