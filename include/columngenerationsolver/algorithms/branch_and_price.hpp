#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

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
     * winning candidate's children are re-solved to full convergence when
     * they are later expanded, unless they already converged under this
     * cap.
     */
    Counter strong_branching_maximum_number_of_iterations = 20;


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
     * 'true' iff the open-node queue was exhausted, i.e. the returned
     * solution (if any) is proven optimal. 'false' means the search was cut
     * short (time/node limit), so 'solution'/'bound' only give a gap, not a
     * proof.
     */
    bool optimal = false;


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
                });
        return json;
    }
};

const BranchAndPriceOutput branch_and_price(
        const Model& model,
        const BranchAndPriceParameters& parameters = {});

}
