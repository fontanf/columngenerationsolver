#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

/** Information given to the node callback of 'branch_and_price'. */
struct BranchAndPriceNodeCallbackInput
{
    /** Relaxation solution of the node, solved to full convergence. */
    const Solution& relaxation_solution;

    /** Dual bound of the node. */
    Value bound;

    /** Depth of the node (0 for the root). */
    Counter depth;

    /** Branching decisions on the path from the root to the node. */
    const std::vector<std::shared_ptr<const BranchingDecision>>& branching_decisions;

    /**
     * Objective cutoff before the call: the best of the input cutoff
     * ('column_generation_parameters.objective_cutoff'), of the cutoffs
     * returned by the previous calls and of the value of the incumbent;
     * +infinity if none.
     */
    Value objective_cutoff;
};

/** Result of the node callback of 'branch_and_price'. */
struct BranchAndPriceNodeCallbackOutput
{
    /**
     * New objective cutoff: no solution that isn't strictly better than it
     * is of interest (sense-aware: strictly less for Minimize, strictly
     * greater for Maximize), e.g. the value of a solution found by the
     * caller, or, for integer objective values, that value minus one plus a
     * tolerance. Used if it is better than the current cutoff;
     * +infinity (the default): no new cutoff.
     */
    Value objective_cutoff = std::numeric_limits<Value>::infinity();

    /** Columns added to the column pool, e.g. those of a solution found by the caller. */
    std::vector<std::shared_ptr<const Column>> columns;
};

/**
 * Callback called once each node is solved to full convergence (and its
 * relaxation is feasible), before the node is pruned or branched on.
 *
 * It can run a primal heuristic from the relaxation solution of the node:
 * a node whose bound reaches the returned cutoff is pruned, so a caller that
 * builds an optimal solution of the node can report its value without
 * building a 'Solution' of the model, nor providing branching candidates for
 * the node.
 */
using BranchAndPriceNodeCallback = std::function<BranchAndPriceNodeCallbackOutput(const BranchAndPriceNodeCallbackInput&)>;

struct BranchAndPriceParameters: Parameters
{
    /** Parameters for the column generation sub-problem. */
    ColumnGenerationParameters column_generation_parameters;

    /**
     * Maximum number of branching candidates to strong-branch evaluate per
     * node (-1: no limit). Candidates come exclusively from
     * 'PricingSolver::compute_branching_candidates' — branch_and_price
     * never falls back to branching on columns, since fixing a column to
     * one exact value doesn't exhaustively partition the search space and
     * so is unsound for an algorithm claiming a proof of optimality. Column
     * branching is exclusively a heuristic technique, used by
     * limited_discrepancy_search.
     */
    Counter maximum_number_of_branching_candidates = -1;

    /**
     * Maximum number of column generation iterations used while
     * strong-branch evaluating a candidate's children (-1: no limit, i.e.
     * evaluate to full convergence). Keeps candidate comparison cheap; the
     * winning candidate's children are always re-solved to full
     * convergence when they are later expanded, regardless of how far this
     * capped evaluation got.
     */
    Counter strong_branching_maximum_number_of_iterations = 20;

    /**
     * Node callback (see 'BranchAndPriceNodeCallback'); none if null.
     *
     * The objective cutoff of the search is the best of
     * 'column_generation_parameters.objective_cutoff', of the cutoffs
     * returned by this callback and of the value of the incumbent: it is
     * given to the column generation of each node, and the nodes whose
     * bound reaches it are pruned. 'BranchAndPriceOutput::optimal' then
     * means that no solution is strictly better than this cutoff.
     */
    BranchAndPriceNodeCallback node_callback = nullptr;


    virtual int format_width() const override { return 47; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Maximum number of branching candidates: " << maximum_number_of_branching_candidates << std::endl
            << std::setw(width) << std::left << "Strong branching maximum number of iterations: " << strong_branching_maximum_number_of_iterations << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                {"MaximumNumberOfBranchingCandidates", maximum_number_of_branching_candidates},
                {"StrongBranchingMaximumNumberOfIterations", strong_branching_maximum_number_of_iterations},
                });
        return json;
    }
};

struct BranchAndPriceOutput: Output
{
    /** Constructor. */
    BranchAndPriceOutput(const Model& model):
        Output(model) { }


    /** Number of nodes explored (popped from the open-node queue). */
    Counter number_of_nodes = 0;

    /** Maximum depth reached. */
    Counter maximum_depth = 0;

    /** Number of branching candidates strong-branch evaluated. */
    Counter number_of_branching_candidates_evaluated = 0;

    /**
     * 'true' iff the open-node queue was exhausted, i.e. no solution is
     * strictly better than the objective cutoff of the search (the returned
     * solution, if any, is proven optimal when it reaches the cutoff).
     * 'false' means the search was cut short (time/node limit), so
     * 'solution'/'bound' only give a gap, not a proof.
     */
    bool optimal = false;

    /**
     * Objective cutoff at the end of the search (see
     * 'BranchAndPriceParameters::node_callback'); +infinity if none.
     */
    Value objective_cutoff = std::numeric_limits<Value>::infinity();


    virtual int format_width() const override { return 30; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Number of nodes: " << number_of_nodes << std::endl
            << std::setw(width) << std::left << "Maximum depth: " << maximum_depth << std::endl
            << std::setw(width) << std::left << "Number of branching candidates evaluated: " << number_of_branching_candidates_evaluated << std::endl
            << std::setw(width) << std::left << "Optimal: " << optimal << std::endl
            << std::setw(width) << std::left << "Objective cutoff: " << objective_cutoff << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                {"NumberOfNodes", number_of_nodes},
                {"MaximumDepth", maximum_depth},
                {"NumberOfBranchingCandidatesEvaluated", number_of_branching_candidates_evaluated},
                {"Optimal", optimal},
                {"ObjectiveCutoff", objective_cutoff},
                });
        return json;
    }
};

const BranchAndPriceOutput branch_and_price(
        const Model& model,
        const BranchAndPriceParameters& parameters = {});

}
