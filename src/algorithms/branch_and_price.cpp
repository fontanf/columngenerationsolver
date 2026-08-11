#include "columngenerationsolver/algorithms/branch_and_price.hpp"

#include "columngenerationsolver/algorithm_formatter.hpp"

#include <set>
#include <cmath>

using namespace columngenerationsolver;

namespace
{

struct BranchAndPriceNode
{
    /** Parent node. */
    std::shared_ptr<BranchAndPriceNode> parent = nullptr;

    /** Depth of the node. */
    ColIdx depth = 0;

    /**
     * Dual bound of the node. Set from a strong-branching evaluation when
     * the node is created (a valid, if possibly loose, bound — column
     * generation bounds are valid at every iteration, not just at
     * convergence), then overwritten once this node is popped and
     * (re-)solved to full convergence.
     */
    Value bound = 0.0;

    /** Relaxation solution of the node, once solved. */
    std::shared_ptr<Solution> relaxation_solution;

    /**
     * Cuts active at this node: exactly this node's own
     * 'ColumnGenerationOutput::cuts' (already the full final active set,
     * self-contained), not a tree-wide pool. A cut separated on one branch
     * isn't necessarily relevant, let alone valid, on an unrelated branch
     * (e.g. a non-robust cut derived using this branch's fixed columns /
     * branching decisions), so only a node's own lineage should ever see
     * it — same reasoning as 'limited_discrepancy_search.cpp'.
     */
    std::vector<std::shared_ptr<const Cut>> cuts;

    /**
     * Branching decision(s) that produce this node from its parent (empty
     * for the root). Branch-and-price never branches on columns — fixing
     * a column to one exact value doesn't exhaustively partition the
     * search space (there's no way to express an inequality bound on a
     * shared, pooled Column), so it's unsound for anything claiming a
     * proof of optimality. Column branching is exclusively a heuristic
     * technique, used by limited_discrepancy_search.
     */
    std::vector<std::shared_ptr<const BranchingDecision>> branching_decisions;
};

}

const BranchAndPriceOutput columngenerationsolver::branch_and_price(
        const Model& model,
        const BranchAndPriceParameters& parameters)
{
    // Initial display.
    BranchAndPriceOutput output(model);
    AlgorithmFormatter algorithm_formatter(
            model,
            parameters,
            output);
    algorithm_formatter.start("Branch-and-price");
    algorithm_formatter.print_column_generation_header();
    output.dummy_column_objective_coefficient = parameters.dummy_column_objective_coefficient;

    bool minimize = (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize);

    std::vector<std::shared_ptr<const Column>> column_pool = parameters.column_pool;

    // Open-node queue, ordered best-bound-first (sense-aware). The global
    // dual bound at any point is the bound of the node at the front of the
    // queue: a valid lower (minimization) / upper (maximization) bound on
    // every currently-open node's subtree, and therefore on whatever
    // hasn't been explored yet.
    auto comp = [minimize](
            const std::shared_ptr<BranchAndPriceNode>& node_1,
            const std::shared_ptr<BranchAndPriceNode>& node_2)
    {
        if (node_1->bound != node_2->bound) {
            return (minimize)?
                node_1->bound < node_2->bound:
                node_1->bound > node_2->bound;
        }
        // Tie-break: deeper nodes first, to help find incumbents sooner.
        return node_1->depth > node_2->depth;
    };
    std::multiset<std::shared_ptr<BranchAndPriceNode>, decltype(comp)> nodes(comp);

    // Root node.
    auto root = std::make_shared<BranchAndPriceNode>();
    root->bound = (minimize)?
        -std::numeric_limits<Value>::infinity():
        +std::numeric_limits<Value>::infinity();
    nodes.insert(root);

    while (!nodes.empty()) {

        // Check end.
        if (parameters.timer.needs_to_end())
            break;

        // Get node.
        auto node = *nodes.begin();
        nodes.erase(nodes.begin());
        output.number_of_nodes++;
        if (output.maximum_depth < node->depth)
            output.maximum_depth = node->depth;

        // Reconstruct this node's branching_decisions by walking its
        // ancestry (like the "Compute fixed_columns and tabu" block in
        // limited_discrepancy_search.cpp), since these are node-local and
        // must never be accumulated globally.
        std::vector<std::shared_ptr<const BranchingDecision>> branching_decisions;
        for (auto node_tmp = node;
                node_tmp->parent != nullptr;
                node_tmp = node_tmp->parent) {
            if (!node_tmp->branching_decisions.empty()) {
                branching_decisions.insert(
                        branching_decisions.end(),
                        node_tmp->branching_decisions.begin(),
                        node_tmp->branching_decisions.end());
            }
        }

        // (Re-)solve this node to full convergence: a strong-branching
        // evaluation may have already set this node's 'bound' (used only
        // to order the queue and score candidates) from a capped, possibly
        // non-convergent column_generation() call — column generation
        // bounds are valid at every iteration, not just at convergence, so
        // it was safe to order the queue by that estimate, but a full,
        // uncapped solve (with cutting planes, if enabled) is always
        // required before this node can be trusted for pruning,
        // integer-feasibility checking, or branching. Always doing this
        // (rather than reusing a strong-branching evaluation that happened
        // to already converge under its iteration cap) also sidesteps
        // having to decide whether cutting planes — which strong-branching
        // evaluations don't even run, see below — would have finished
        // separating by the time the cap hit.
        {
            ColumnGenerationParameters column_generation_parameters
                = parameters.column_generation_parameters;
            column_generation_parameters.timer = parameters.timer;
            column_generation_parameters.verbosity_level = 0;
            column_generation_parameters.dummy_column_objective_coefficient
                = output.dummy_column_objective_coefficient;
            if (node->parent == nullptr) {
                column_generation_parameters.initial_columns = parameters.initial_columns;
            } else if (node->parent->relaxation_solution != nullptr) {
                for (const auto& p: node->parent->relaxation_solution->columns()) {
                    bool ok = true;
                    for (const auto& column: model.static_columns)
                        if (p.first.get() == column.get())
                            ok = false;
                    if (ok)
                        column_generation_parameters.initial_columns.push_back(p.first);
                }
            }
            column_generation_parameters.column_pool = column_pool;
            column_generation_parameters.initial_cuts = (node->parent == nullptr)?
                parameters.initial_cuts:
                node->parent->cuts;
            column_generation_parameters.fixed_columns = parameters.fixed_columns;
            column_generation_parameters.branching_decisions = branching_decisions;

            auto cg_output = column_generation(
                    model,
                    column_generation_parameters);

            output.time_lpsolve += cg_output.time_lpsolve;
            output.time_pricing += cg_output.time_pricing;
            output.dummy_column_objective_coefficient = cg_output.dummy_column_objective_coefficient;
            output.number_of_column_generation_iterations += cg_output.number_of_column_generation_iterations;
            output.columns.insert(
                    output.columns.end(),
                    cg_output.columns.begin(),
                    cg_output.columns.end());
            column_pool.insert(
                    column_pool.end(),
                    cg_output.columns.begin(),
                    cg_output.columns.end());
            node->cuts = cg_output.cuts;

            if (parameters.timer.needs_to_end())
                break;

            if (!cg_output.relaxation_solution_is_feasible) {
                // Infeasible: prune, no children.
                continue;
            }

            node->bound = cg_output.bound;
            node->relaxation_solution = std::shared_ptr<Solution>(new Solution(cg_output.relaxation_solution));

            if (node->parent == nullptr) {
                algorithm_formatter.print_header();
                output.relaxation_solution = *node->relaxation_solution;
            }
        }

        // Report the global bound. This node had the best bound among all
        // open nodes *at the time it was popped* — but resolving it above
        // to full convergence may have revised its bound upward (worse,
        // for minimization), since it was only selected using a loose
        // strong-branching estimate. That revised bound is not necessarily
        // the true minimum over everything still open, so compare against
        // whatever now sits at the front of the queue (itself a valid,
        // if possibly still loose, lower bound) and report the better of
        // the two — never just the popped node in isolation.
        Value global_bound = node->bound;
        if (!nodes.empty()) {
            global_bound = (minimize)?
                (std::min)(global_bound, (*nodes.begin())->bound):
                (std::max)(global_bound, (*nodes.begin())->bound);
        }
        algorithm_formatter.update_bound(global_bound);

        std::stringstream ss;
        ss << "node " << output.number_of_nodes
            << " depth " << node->depth
            << " bound " << node->bound;

        // Prune if this node cannot improve on the incumbent.
        if (output.solution.feasible()) {
            if (minimize && node->bound >= output.solution.objective_value() - FFOT_TOL) {
                algorithm_formatter.print(ss.str());
                continue;
            }
            if (!minimize && node->bound <= output.solution.objective_value() + FFOT_TOL) {
                algorithm_formatter.print(ss.str());
                continue;
            }
        }

        // If the relaxation is already integer feasible, it's a new
        // incumbent; no children.
        if (node->relaxation_solution->feasible()) {
            algorithm_formatter.update_solution(*node->relaxation_solution);
            algorithm_formatter.print(ss.str());
            continue;
        }

        algorithm_formatter.print(ss.str());

        // Determine branching candidates. Branch-and-price never falls
        // back to branching on columns (that's exclusively a heuristic
        // technique — see the note on 'BranchAndPriceNode::
        // branching_decisions'): the pricing solver must provide branching
        // candidates for every fractional relaxation it can be handed.
        auto decision_candidates = model.pricing_solver->compute_branching_candidates(
                *node->relaxation_solution);
        if (decision_candidates.empty()) {
            throw std::runtime_error(
                    "columngenerationsolver::branch_and_price: "
                    "'PricingSolver::compute_branching_candidates' returned no "
                    "candidates for a fractional relaxation. branch_and_price "
                    "requires the pricing solver to implement branching; unlike "
                    "the heuristic algorithms, it never falls back to branching "
                    "on columns.");
        }

        // Keep only the highest-scored candidates when there are more than
        // can be strong-branch evaluated. A stable sort keeps
        // compute_branching_candidates' original relative order among
        // tied (e.g. left-at-default) scores.
        if (parameters.maximum_number_of_branching_candidates != -1
                && (Counter)decision_candidates.size()
                > parameters.maximum_number_of_branching_candidates) {
            std::stable_sort(
                    decision_candidates.begin(),
                    decision_candidates.end(),
                    [](
                        const BranchingCandidate& candidate_1,
                        const BranchingCandidate& candidate_2)
                    {
                        return candidate_1.score > candidate_2.score;
                    });
            decision_candidates.resize(parameters.maximum_number_of_branching_candidates);
        }

        std::vector<std::vector<std::shared_ptr<BranchAndPriceNode>>> node_candidates;
        for (const auto& candidate: decision_candidates) {
            std::vector<std::shared_ptr<BranchAndPriceNode>> children;
            for (const auto& decision: candidate.branching_decisions) {
                auto child = std::make_shared<BranchAndPriceNode>();
                child->parent = node;
                child->depth = node->depth + 1;
                child->branching_decisions = {decision};
                children.push_back(child);
            }
            node_candidates.push_back(children);
        }

        // Strong-branching evaluation: solve every child of every candidate
        // with a capped column_generation() call, score each candidate by
        // its worst (smallest) bound degradation across children, and keep
        // only the winning candidate's children.
        Value best_score = -std::numeric_limits<Value>::infinity();
        std::vector<std::shared_ptr<BranchAndPriceNode>> best_children;
        for (const auto& children: node_candidates) {
            Value score = std::numeric_limits<Value>::infinity();
            for (const auto& child: children) {
                std::vector<std::shared_ptr<const BranchingDecision>> child_branching_decisions
                    = branching_decisions;
                child_branching_decisions.insert(
                        child_branching_decisions.end(),
                        child->branching_decisions.begin(),
                        child->branching_decisions.end());

                ColumnGenerationParameters column_generation_parameters
                    = parameters.column_generation_parameters;
                column_generation_parameters.timer = parameters.timer;
                column_generation_parameters.verbosity_level = 0;
                column_generation_parameters.dummy_column_objective_coefficient
                    = output.dummy_column_objective_coefficient;
                for (const auto& p: node->relaxation_solution->columns()) {
                    bool ok = true;
                    for (const auto& column: model.static_columns)
                        if (p.first.get() == column.get())
                            ok = false;
                    if (ok)
                        column_generation_parameters.initial_columns.push_back(p.first);
                }
                column_generation_parameters.column_pool = column_pool;
                column_generation_parameters.initial_cuts = node->cuts;
                column_generation_parameters.fixed_columns = parameters.fixed_columns;
                column_generation_parameters.branching_decisions = child_branching_decisions;
                column_generation_parameters.maximum_number_of_iterations
                    = parameters.strong_branching_maximum_number_of_iterations;
                // Strong-branching evaluations are capped, throwaway
                // estimates used only to score candidates; separating cuts
                // for them isn't worth the cost, and since a node's bound
                // and relaxation solution are always fully re-solved (with
                // cutting planes, if enabled) once it's actually popped
                // (see above), nothing here depends on this evaluation
                // having any cuts beyond the ones already active at the
                // parent.
                column_generation_parameters.cutting_planes = 0;

                auto cg_output = column_generation(
                        model,
                        column_generation_parameters);

                output.time_lpsolve += cg_output.time_lpsolve;
                output.time_pricing += cg_output.time_pricing;
                output.number_of_column_generation_iterations += cg_output.number_of_column_generation_iterations;
                output.columns.insert(
                        output.columns.end(),
                        cg_output.columns.begin(),
                        cg_output.columns.end());
                column_pool.insert(
                        column_pool.end(),
                        cg_output.columns.begin(),
                        cg_output.columns.end());

                if (parameters.timer.needs_to_end())
                    break;

                // Branching only restricts the feasible region, so the
                // child's true bound can never be looser than its parent's
                // — but a capped, partially-converged evaluation can
                // *report* something looser purely because it hasn't
                // caught up yet (column generation bounds are valid at
                // every iteration, just not yet tight). Clamp to the
                // parent's bound: still valid (the true value is at least
                // this tight), and avoids polluting the open-node queue
                // with under-iterated, misleadingly loose bounds. This is
                // only ever used to order the queue and score candidates —
                // once popped, every node is always fully re-solved (see
                // above), so there's no need to also carry over this
                // evaluation's (possibly non-convergent) relaxation
                // solution or cuts.
                child->bound = (minimize)?
                    (std::max)(cg_output.bound, node->bound):
                    (std::min)(cg_output.bound, node->bound);

                Value degradation = (minimize)?
                    child->bound - node->bound:
                    node->bound - child->bound;
                score = (std::min)(score, degradation);
            }

            if (parameters.timer.needs_to_end())
                break;

            output.number_of_branching_candidates_evaluated++;
            if (score > best_score) {
                best_score = score;
                best_children = children;
            }
        }

        if (parameters.timer.needs_to_end())
            break;

        for (const auto& child: best_children)
            nodes.insert(child);
    }

    output.optimal = nodes.empty() && !parameters.timer.needs_to_end();

    algorithm_formatter.end();
    return output;
}
