#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct LimitedDiscrepancySearchParameters: Parameters
{
    /** Maximum discrepancy. */
    Value discrepancy_limit = std::numeric_limits<Value>::infinity();

    /**
     * Automatic stop: if finite, the search stops once it has explored at
     * least 'automatic_stop_minimum_number_of_nodes' nodes and more than
     * 'automatic_stop_factor' times the maximum depth reached (the depth
     * minus the discrepancy of the deepest node). Infinite by default: no
     * automatic stop.
     *
     * Since the number of nodes of the first dive is its depth, with a factor
     * of at least 1, the first dive always reaches a leaf before the search
     * stops. Increasing the factor between successive searches lets a caller
     * restart the search with a larger node limit.
     */
    double automatic_stop_factor = std::numeric_limits<double>::infinity();

    /** Minimum number of nodes before the automatic stop (see 'automatic_stop_factor'). */
    Counter automatic_stop_minimum_number_of_nodes = 64;

    bool continue_until_feasible = false;

    /**
     * Prune the nodes whose relaxation is no better than the best solution
     * found by the search.
     */
    bool bound = false;

    /**
     * Objective cutoff: the search need not find anything that is not
     * strictly better than this value (sense-aware: strictly less for
     * Minimize, strictly greater for Maximize), for example the value of a
     * solution found by the caller elsewhere. Defaults to +infinity, a
     * sentinel meaning "no cutoff", for both objective senses.
     *
     * At each node, the cutoff is passed to the column generation (see
     * 'ColumnGenerationParameters::objective_cutoff'), together with the
     * value of the best solution found by the search if 'bound' is enabled,
     * and the node is pruned if its bound or the value of its relaxation
     * reaches it.
     */
    Value objective_cutoff = std::numeric_limits<Value>::infinity();

    /** Parameters for the column generation sub-problem. */
    ColumnGenerationParameters column_generation_parameters;


    virtual int format_width() const override { return 39; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Discrepancy limit: " << discrepancy_limit << std::endl
            << std::setw(width) << std::left << "Automatic stop factor: " << automatic_stop_factor << std::endl
            << std::setw(width) << std::left << "Automatic stop min. # nodes: " << automatic_stop_minimum_number_of_nodes << std::endl
            << std::setw(width) << std::left << "Continue until feasible: " << continue_until_feasible << std::endl
            << std::setw(width) << std::left << "Bound: " << bound << std::endl
            << std::setw(width) << std::left << "Objective cutoff: " << objective_cutoff << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                {"DiscrepancyLimit", discrepancy_limit},
                {"AutomaticStopFactor", automatic_stop_factor},
                {"AutomaticStopMinimumNumberOfNodes", automatic_stop_minimum_number_of_nodes},
                {"ContinueUntilFeasible", continue_until_feasible},
                {"Bound", bound},
                {"ObjectiveCutoff", objective_cutoff},
                });
        return json;
    }
};

struct LimitedDiscrepancySearchOutput: Output
{
    /** Constructor. */
    LimitedDiscrepancySearchOutput(const Model& model):
        Output(model),
        root_relaxation_solution(SolutionBuilder().set_model(model).build()) { }


    Counter number_of_nodes = 0;

    Counter maximum_depth = 0;

    Value maximum_discrepancy = -1;

    /** The root node's relaxation solution. */
    Solution root_relaxation_solution;

    /** Cuts still active at the end of the root node's relaxation. */
    std::vector<std::shared_ptr<const Cut>> root_cuts;


    virtual int format_width() const override { return 30; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Number of nodes: " << number_of_nodes << std::endl
            << std::setw(width) << std::left << "Maximum depth: " << maximum_depth << std::endl
            << std::setw(width) << std::left << "Maximum discrepancy: " << maximum_discrepancy << std::endl
            << std::setw(width) << std::left << "Number of root cuts: " << root_cuts.size() << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                {"NumberOfNodes", number_of_nodes},
                {"MaximumDepth", maximum_depth},
                {"MaximumDiscrepancy", maximum_discrepancy},
                {"NumberOfRootCuts", root_cuts.size()},
                });
        return json;
    }
};

const LimitedDiscrepancySearchOutput limited_discrepancy_search(
        const Model& model,
        const LimitedDiscrepancySearchParameters& parameters = {});

}
