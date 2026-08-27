#pragma once

#include "optimizationtools/utils/output.hpp"
#include "optimizationtools/utils/utils.hpp"
#include "optimizationtools/containers/indexed_map.hpp"

#include <vector>
#include <cstdint>
#include <iomanip>
#include <unordered_set>

namespace columngenerationsolver
{

using Counter = int64_t;
using ColIdx = int64_t;
using RowIdx = int64_t;
using CutIdx = int64_t;
using Value = double;

enum class VariableType { Continuous, Integer };

struct LinearTerm
{
    /** Row index. */
    RowIdx row;

    /** Coefficient. */
    Value coefficient;
};

/**
 * Structure for a column.
 */
struct Column
{
    /** Column name. */
    std::string name;

    /** Type. */
    VariableType type = VariableType::Integer;

    /** Lower bound. */
    Value lower_bound = 0.0;

    /** Upper bound. */
    Value upper_bound = std::numeric_limits<Value>::infinity();

    /** Coefficient in the objective. */
    Value objective_coefficient = 0;

    /** Row indices. */
    std::vector<LinearTerm> elements;

    /** Branching priority. */
    Value branching_priority = 0;

    /**
     * Extra information.
     *
     * This field may be used to retrieve the real solution from the column.
     * For example, if a column represent a path, the order in which the
     * elements of the path are visited may be stored in this attribute.
     */
    std::shared_ptr<void> extra;
};

inline std::ostream& operator<<(
        std::ostream& os,
        const Column& column)
{
    os << "name: " << column.name << std::endl;
    os << "objective coefficient: " << column.objective_coefficient << std::endl;
    os << "lower bound: " << column.lower_bound << std::endl;
    os << "upper bound: " << column.upper_bound << std::endl;
    os << "row indices:";
    for (RowIdx row_pos = 0;
            row_pos < (RowIdx)column.elements.size();
            ++row_pos) {
        os << " " << column.elements[row_pos].row;
    }
    os << std::endl;
    os << "row coefficients:";
    for (RowIdx row_pos = 0;
            row_pos < (RowIdx)column.elements.size();
            ++row_pos) {
        os << " " << column.elements[row_pos].coefficient;
    }
    return os;
}

/**
 * Structure for a row.
 */
struct Row
{
    /** Row name. */
    std::string name;

    /** Lower bounds of the constraints. */
    Value lower_bound = 0.0;

    /** Upper bounds of the constraints. */
    Value upper_bound = 0.0;

    /**
     * Lower bounds of the coefficicients of the variables to generate for each
     * constraint.
     */
    Value coefficient_lower_bound = 0.0;

    /**
     * Upper bounds of the coefficicients of the variables to generate for each
     * constraint.
     */
    Value coefficient_upper_bound = 1.0;

    /**
     * Tolerance used when checking whether the row is already satisfied by
     * fixed columns. Should reflect the scale of the constraint coefficients.
     */
    Value feasibility_tolerance = 0.0;
};

class Solution;

/**
 * A cutting plane.
 *
 * Unlike 'Row', a cut's coefficient on a column is not necessarily
 * decomposable from static per-column data: non-robust cuts (e.g.
 * subset-row cuts) require cut-family-specific logic to compute it. That
 * logic lives on 'PricingSolver::coefficient' rather than on 'Cut' itself,
 * so 'Cut' stays a concrete, uniform type like 'Column': cut families carry
 * whatever data they need through 'extra' instead of subclassing.
 *
 * 'PricingSolver::coefficient' only serves master problem bookkeeping
 * (building the LP, rechecking pooled columns' reduced costs); it is not
 * used by the pricing solver's internal search. A pricing solver that
 * wants to enforce a non-robust cut during pricing needs direct, typed
 * access to the active 'Cut' objects themselves, available via
 * 'PricingSolver::solve_pricing's 'cut_duals' (each paired with its dual
 * value), since a coefficient computed on a finished column can't inform
 * search-time pruning of an incomplete one.
 */
struct Cut
{
    /** Cut name. */
    std::string name;

    /** Lower bound of the cut. */
    Value lower_bound = -std::numeric_limits<Value>::infinity();

    /** Upper bound of the cut. */
    Value upper_bound = 0.0;

    /**
     * Tolerance used when checking whether the cut is still active, i.e.
     * whether its value at the current relaxation solution has slack (on
     * both sides, relative to 'lower_bound'/'upper_bound') beyond this
     * tolerance, making it a candidate for removal from the active set —
     * the same check 'Row::feasibility_tolerance' does for rows. Should
     * reflect the scale of the cut's coefficients, which only the code
     * that creates the cut knows; defaults to 0 (a cut is only ever
     * removed if it has no slack at all), so cut families are opted into
     * automatic removal explicitly rather than by a framework-guessed
     * default.
     */
    Value feasibility_tolerance = 1e-3;

    /**
     * Extra information.
     *
     * Problem-specific data needed to compute this cut's coefficients and
     * to recognize it against other cuts, read back by 'PricingSolver::
     * coefficient' and 'PricingSolver::equal' via a 'static_cast' to the
     * concrete type the code that built the cut knows about. Same idiom as
     * 'Column::extra'.
     */
    std::shared_ptr<void> extra;
};

/**
 * A branching decision, for branch-and-price.
 *
 * Concrete, non-polymorphic, like 'Column': unlike 'Cut', the framework
 * never invokes any behavior on a 'BranchingDecision' — it only threads it
 * through node ancestry and hands it to 'PricingSolver::solve_pricing'
 * (and 'PricingSolver::infeasible_columns'), so there is no
 * framework-required virtual method to dispatch on. Family-specific data
 * (e.g. which two customers a Ryan-Foster decision is defined over) goes
 * in 'extra', for the same 'PricingSolver' that created the decision (via
 * 'compute_branching_candidates') to read back when it later receives it
 * again there.
 */
class BranchingDecision
{

public:

    /** Branching decision name. */
    std::string name;

    /** Branching-decision-family-specific data. */
    std::shared_ptr<void> extra;
};

/**
 * A branch-and-price branching candidate: the decisions defining each of
 * its children (one per child), plus a static score used to rank
 * candidates when there are more of them than
 * 'BranchAndPriceParameters::maximum_number_of_branching_candidates' can
 * afford to strong-branch evaluate. Higher scores are more promising.
 * Candidates with a tied (e.g. left-at-default) score keep the relative
 * order 'PricingSolver::compute_branching_candidates' returned them in.
 */
struct BranchingCandidate
{
    /** Decisions defining each child of this candidate. */
    std::vector<std::shared_ptr<const BranchingDecision>> branching_decisions;

    /** Static score used to rank candidates before strong-branching evaluation. */
    Value score = 0.0;
};

/**
 * Interface for the pricing problem solver.
 */
class PricingSolver
{

public:

    virtual ~PricingSolver() { }

    struct PricingOutput
    {
        /** Newly-found columns. */
        std::vector<std::shared_ptr<const Column>> columns;

        /**
         * Bound on the best possible reduced cost among all columns of the
         * pricing subproblem, not just the ones actually returned in
         * 'columns'. Three ways a 'PricingSolver' can behave, in
         * increasing order of usefulness for the master problem's dual
         * bound:
         * - Exact: always sets 'overcost' to the true optimal reduced
         *   cost, whether or not it's negative (minimization) / positive
         *   (maximization) enough to return as a column.
         * - Heuristic, no bound: leaves 'overcost' at its infinity
         *   default, even when columns are found — the framework then
         *   can't tighten the dual bound from this pricing call at all.
         * - Heuristic, with a bound: sets 'overcost' to a genuine bound
         *   obtained by other means (e.g. a relaxation of the pricing
         *   subproblem), even if the returned columns don't attain it.
         *   Example: for a rectangle-packing pricing subproblem (a 2D
         *   knapsack), a heuristic can find a column, while 'overcost'
         *   comes from the pricing subproblem's 1D relaxation.
         *
         * Under 'solve_pricing(solve_feasibility=true, ...)', the same
         * three-way contract applies, but against the *zeroed* objective:
         * 'overcost' bounds the best reduced cost with every column's real
         * 'objective_coefficient' treated as 0, not the real one.
         */
        Value overcost = std::numeric_limits<Value>::infinity();

        /**
         * Aggregate column contributions Σ_k A·z*_k across all subproblems,
         * used to compute the subgradient for Wentges/directional smoothing.
         *
         * Leave empty to let the framework sum the returned columns (correct
         * when each independent subproblem returns exactly one column).
         * Set explicitly when that assumption does not hold — e.g. for
         * identical subproblems (bin packing with N bins): set each entry to
         * N * A[row, z*] so the subgradient reflects all N subproblems, just
         * as overcost = N * rc* reflects them for the dual bound.
         */
        std::vector<Value> lagrangian_column_values;
    };

    /**
     * Which of 'columns' (the model's static columns plus whatever
     * generated columns are being carried over into this attempt as a
     * warm start) 'fixed_columns' or 'branching_decisions' rule out --
     * filtered out of the master LP entirely, before it's even built from
     * these column sets.
     *
     * The two intended uses are both cases where a constraint exists but
     * was deliberately never turned into a master 'Row' (so it has no
     * dual, and 'solve_pricing' can't be steered by it the way it's
     * steered by real duals):
     * - A linking/coupling constraint between subproblems that's cheaper
     *   to enforce by excluding the specific columns that would violate
     *   it than by adding it as an explicit row (e.g. "the same bin/
     *   machine can't be claimed by two different fixed columns").
     * - A branch-and-price branching decision (e.g. Ryan-Foster). Its own
     *   effect on 'solve_pricing' only stops *new* violating columns from
     *   being generated going forward; it says nothing about columns that
     *   already exist in 'columns' from before the decision was made
     *   (static columns, or ones warm-started in from an ancestor node)
     *   and now violate it -- this is what retroactively excludes those.
     *
     * Called once per column generation attempt, not once per
     * 'solve_pricing' call, so it's the wrong place for any setup a
     * 'PricingSolver' wants to reuse across those calls -- pass whatever
     * it needs directly to 'solve_pricing' instead (fixed columns and
     * branching decisions are given there too). Default implementation
     * returns none, for a 'PricingSolver' whose columns are never
     * affected this way.
     */
    virtual std::vector<std::shared_ptr<const Column>> infeasible_columns(
            const std::vector<std::shared_ptr<const Column>>& columns,
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns,
            const std::vector<std::shared_ptr<const BranchingDecision>>& branching_decisions) const
    {
        (void)columns;
        (void)fixed_columns;
        (void)branching_decisions;
        return {};
    }

    /**
     * Number of pricing levels this solver supports (>= 1), e.g. a fast
     * heuristic (level 0), a more expensive heuristic, an exact algorithm,
     * ... ordered from cheapest/least thorough to most expensive/thorough.
     * 'column_generation' starts every call at level 0 and only escalates
     * once a whole cutting-plane fixed point is reached at the current
     * level (no new column, no new or removed cut) — see 'solve_pricing'.
     * The default of 1 (single level) means it never escalates, i.e. a
     * solver that doesn't override this is unaffected.
     */
    virtual Counter number_of_pricing_levels() const
    {
        return 1;
    }

    /**
     * Solve the pricing subproblem.
     *
     * When 'solve_feasibility' is 'true', search for columns that improve
     * feasibility only: optimize purely 'duals·column' (plus 'cut_duals'
     * contributions), ignoring each candidate's real
     * 'objective_coefficient'. Still return ordinary 'Column' objects with
     * their real 'objective_coefficient' field intact — it's just not what
     * the search itself optimized for. See 'PricingOutput::overcost' for
     * how this affects the bound contract.
     *
     * 'pricing_level' (see 'number_of_pricing_levels') selects how
     * thorough the search should be, independently of 'solve_feasibility'
     * -- both phases escalate through the same levels together.
     *
     * 'fixed_columns', 'branching_decisions' and 'tabu' are given on every
     * call rather than once via a separate setup step: within one column
     * generation attempt they're the same on every call (only 'duals'/
     * 'cut_duals'/'pricing_level' change call to call), and how cheaply a
     * 'PricingSolver' can fold fixed/tabu state into its search dwarfs in
     * cost next to the search itself for every solver in this repository,
     * so there's nothing worth amortizing across calls by caching it from
     * a prior setup call. A 'PricingSolver' that does need to derive
     * something expensive from them is free to cache it keyed on the
     * columns' own pointer identity (they're the same 'shared_ptr's
     * across calls within one attempt) rather than recomputing
     * unconditionally. Active cuts aren't repeated here since 'cut_duals'
     * already pairs each one with its dual value. Honoring 'tabu' is
     * optional -- the framework filters tabu columns out of what it adds
     * to the master LP regardless.
     */
    virtual PricingOutput solve_pricing(
            bool solve_feasibility,
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns,
            const std::vector<std::shared_ptr<const BranchingDecision>>& branching_decisions,
            const std::unordered_set<std::shared_ptr<const Column>>& tabu,
            const std::vector<Value>& duals,
            const std::vector<std::pair<std::shared_ptr<const Cut>, Value>>& cut_duals,
            Counter pricing_level) = 0;

    /**
     * Separate cutting planes from the current relaxation solution.
     *
     * Called by 'column_generation' once the relaxation has converged, when
     * cutting planes are enabled -- either genuinely feasible (Phase 2,
     * dummy-column-free by construction), or, if Phase 1 (feasibility)
     * fails to reach one but isn't proven infeasible either, from that
     * inconclusive attempt's own relaxation solution instead: a partial
     * solution, since dummy columns are excluded from it by construction.
     * A cut found this way might let a later Phase 1 attempt restore
     * feasibility where this one couldn't. The default implementation
     * generates no cuts.
     */
    virtual std::vector<std::shared_ptr<const Cut>> separate_cuts(
            const Solution& solution)
    {
        (void)solution;
        return {};
    }

    /**
     * Coefficient of a column in a cut.
     *
     * See 'Cut' for why this lives here rather than on 'Cut' itself. Only
     * called by 'column_generation' for cuts that are actually part of the
     * active set (master problem bookkeeping — see 'Cut'), so a
     * 'PricingSolver' that doesn't use cuts never needs to override it.
     * Throws by default, for the same reason as 'equal' below.
     */
    virtual Value coefficient(
            const Cut& cut,
            const Column& column) const
    {
        (void)cut;
        (void)column;
        throw std::logic_error(
                "columngenerationsolver::PricingSolver::coefficient: "
                "not implemented.");
    }

    /**
     * Reduced cost of 'column', given 'duals' and (optionally)
     * 'cut_duals'.
     *
     * A convenience for a 'PricingSolver' to score candidate columns
     * (typically to decide whether one qualifies, or to rank several).
     * Calls 'coefficient' above for the cut contribution, so 'cut_duals'
     * must stay empty unless 'coefficient' is overridden.
     *
     * When 'solve_feasibility' is 'true', computed against the *zeroed*
     * objective (see 'solve_pricing'/'PricingOutput::overcost'), i.e.
     * 'column.objective_coefficient' is treated as 0 rather than read.
     */
    Value compute_reduced_cost(
            bool solve_feasibility,
            const Column& column,
            const std::vector<Value>& duals,
            const std::vector<std::pair<std::shared_ptr<const Cut>, Value>>& cut_duals = {}) const
    {
        Value reduced_cost = (solve_feasibility)? 0: column.objective_coefficient;
        for (const LinearTerm& element: column.elements)
            reduced_cost -= duals[element.row] * element.coefficient;
        for (const auto& p: cut_duals)
            reduced_cost -= p.second * coefficient(*p.first, column);
        return reduced_cost;
    }

    /**
     * Return whether two cuts are the same constraint.
     *
     * 'column_generation' uses this to recognize a cut it has previously
     * removed from the active set for being inactive, even though
     * 'separate_cuts' may return a different 'Cut' instance for the same
     * constraint each time it becomes violated again — pointer identity
     * cannot be relied on for that. Only called once a cut has already
     * been removed at least once in the current call, so a 'PricingSolver'
     * that doesn't use cuts, or whose cuts are never removed, never
     * triggers it and never needs to override it. Throws by default so
     * that a 'PricingSolver' that does need it fails loudly if it forgets
     * to override it, rather than silently falling back to an incorrect
     * comparison.
     */
    virtual bool equal(
            const Cut& cut_1,
            const Cut& cut_2) const
    {
        (void)cut_1;
        (void)cut_2;
        throw std::logic_error(
                "columngenerationsolver::PricingSolver::equal: "
                "not implemented.");
    }

    /**
     * Propose branching candidates from the current (fractional) relaxation
     * solution, for branch-and-price.
     *
     * Called by 'branch_and_price' when a node's relaxation is not integer
     * feasible. Each returned 'BranchingCandidate' is one candidate to
     * strong-branch over, holding the branching decisions defining its
     * children (not necessarily 2, so N-ary branching is supported) and a
     * score used to rank candidates when there are more than
     * 'BranchAndPriceParameters::maximum_number_of_branching_candidates'.
     * 'branch_and_price' never falls back to branching on columns, so the
     * default implementation proposing no candidates only works for
     * problems whose root relaxation is already integer feasible.
     */
    virtual std::vector<BranchingCandidate> compute_branching_candidates(
            const Solution& solution)
    {
        (void)solution;
        return {};
    }
};

/**
 * Structure for the (expenential) model.
 */
struct Model
{
    /** Objective sense. */
    optimizationtools::ObjectiveDirection objective_sense = optimizationtools::ObjectiveDirection::Minimize;

    /** Constraints. */
    std::vector<Row> rows;

    /** Solver of the pricing problem. */
    std::unique_ptr<PricingSolver> pricing_solver = NULL;

    /** Reduced cost of 'column', given 'duals' and (optionally) 'cut_duals'. */
    Value compute_reduced_cost(
            bool solve_feasibility,
            const Column& column,
            const std::vector<Value>& duals,
            const std::vector<std::pair<std::shared_ptr<const Cut>, Value>>& cut_duals = {}) const
    {
        return pricing_solver->compute_reduced_cost(solve_feasibility, column, duals, cut_duals);
    }

    /** Columns which are not dynamically generated. */
    std::vector<std::shared_ptr<const Column>> static_columns;


    void check_column(
            const std::shared_ptr<const Column>& column) const
    {
        for (const LinearTerm& element: column->elements) {
            if (element.row < 0 || element.row >= rows.size()) {
                std::stringstream ss;
                ss << "Column check failed." << std::endl
                    << "Column:" << std::endl << *column << std::endl
                    << "Invalid row index." << std::endl;
                throw std::runtime_error(ss.str());
            }
        }
    }

    void check_generated_column(
            const std::shared_ptr<const Column>& column) const
    {
        if (column->lower_bound != 0) {
            std::stringstream ss;
            ss << "Generated column check failed." << std::endl
                << "Column:" << std::endl << *column << std::endl
                << "A generated column must have a zero lower bound." << std::endl;
            throw std::runtime_error(ss.str());
        }
        if (column->upper_bound != std::numeric_limits<Value>::infinity()) {
            std::stringstream ss;
            ss << "Generated column check failed." << std::endl
                << "Column:" << std::endl << *column << std::endl
                << "A generated column must have an infinite upper bound." << std::endl;
            throw std::runtime_error(ss.str());
        }
        check_column(column);
    }


    virtual void format(
            std::ostream& os,
            int verbosity_level = 1) const
    {
        if (verbosity_level >= 1) {
            os
                << "Objective sense:           " << ((objective_sense == optimizationtools::ObjectiveDirection::Minimize)? "Minimize": "Maximize") << std::endl
                << "Number of constraints:     " << rows.size() << std::endl
                << "Number of static columns:  " << static_columns.size() << std::endl
                ;
        }

        if (verbosity_level >= 2) {
            os
                << std::endl
                << std::setw(12) << "Row"
                << std::setw(36) << "Name"
                << std::setw(12) << "Lower"
                << std::setw(12) << "Upper"
                << std::endl
                << std::setw(12) << "---"
                << std::setw(36) << "-----"
                << std::setw(12) << "-----"
                << std::setw(12) << "-----"
                << std::endl;
            for (RowIdx row_id = 0;
                    row_id < (RowIdx)rows.size();
                    ++row_id) {
                const Row& row = this->rows[row_id];
                os
                    << std::setw(12) << row_id
                    << std::setw(36) << row.name
                    << std::setw(12) << row.lower_bound
                    << std::setw(12) << row.upper_bound
                    << std::endl;
            }
        }

        if (verbosity_level >= 3) {

            std::vector<std::vector<std::pair<const Column*, Value>>> row_elements(this->rows.size() + 1);
            for (const auto& column: static_columns) {
                if (column->objective_coefficient != 0.0)
                    row_elements[this->rows.size()].push_back({column.get(), column->objective_coefficient});
                for (const LinearTerm& element: column->elements)
                    row_elements[element.row].push_back({column.get(), element.coefficient});
            }
            // Print objective.
            bool first = true;
            os << "- -1 Obj:";
            for (const auto& p: row_elements[this->rows.size()]) {
                const Column* column = p.first;
                Value coef = column->objective_coefficient;
                if (first) {
                    first = false;
                    if (coef == 1) {
                        os << " " << column->name;
                    } else if (coef == -1) {
                        os << " - " << column->name;
                    } else if (coef > 0) {
                        os << " " << coef << " " << column->name;
                    } else {
                        os << " - " << -coef << " " << column->name;
                    }
                } else {
                    if (coef == 1) {
                        os << " + " << column->name;
                    } else if (coef == -1) {
                        os << " - " << column->name;
                    } else if (coef > 0) {
                        os << " + " << coef << " " << column->name;
                    } else {
                        os << " - " << -coef << " " << column->name;
                    }
                }
            }
            os << std::endl;
            // Print rows.
            for (RowIdx row_id = 0;
                    row_id < (RowIdx)rows.size();
                    ++row_id) {
                const Row& row = this->rows[row_id];
                os << "- " << row_id << " " << row.name << ":";
                if (row.upper_bound != std::numeric_limits<Value>::infinity()
                        && row.lower_bound != -std::numeric_limits<Value>::infinity()) {
                    os << " " << row.lower_bound << " <=";
                }

                bool first = true;
                for (const auto& p: row_elements[row_id]) {
                    const Column* column = p.first;
                    Value coef = p.second;
                    if (coef == 0)
                        continue;
                    if (first) {
                        first = false;
                        if (coef == 1) {
                            os << " " << column->name;
                        } else if (coef == -1) {
                            os << " - " << column->name;
                        } else if (coef > 0) {
                            os << " " << coef << " " << column->name;
                        } else {
                            os << " - " << -coef << " " << column->name;
                        }
                    } else {
                        if (coef == 1) {
                            os << " + " << column->name;
                        } else if (coef == -1) {
                            os << " - " << column->name;
                        } else if (coef > 0) {
                            os << " + " << coef << " " << column->name;
                        } else {
                            os << " - " << -coef << " " << column->name;
                        }
                    }
                }

                if (row.upper_bound != std::numeric_limits<Value>::infinity()) {
                    os << " <= " << row.upper_bound;
                } else {
                    os << " >= " << row.lower_bound;
                }
                os << std::endl;
            }
            os << std::endl;
        }
    }
};

class ColumnMap
{

public:

    /** Get columns. */
    const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& columns() const { return columns_; };

    bool contains(
            const std::shared_ptr<const Column>& column)
    {
        return (columns_map_.find(column) != columns_map_.end());
    }

    Value get_column_value(
            const std::shared_ptr<const Column>& column,
            Value default_value = 0) const
    {
        auto it = columns_map_.find(column);
        if (it == columns_map_.end())
            return default_value;
        Counter pos = it->second;
        return columns_[pos].second;
    }

    /** Add a column to the solution. */
    void set_column_value(
            const std::shared_ptr<const Column>& column,
            Value value)
    {
        if (columns_map_.find(column) == columns_map_.end()) {
            columns_map_[column] = columns_.size();
            columns_.push_back({column, value});
        } else {
            Counter pos = columns_map_[column];
            columns_[pos].second = value;
        }
    }

    void max_column_value(
            const std::shared_ptr<const Column>& column,
            Value value)
    {
        if (columns_map_.find(column) == columns_map_.end()) {
            columns_map_[column] = columns_.size();
            columns_.push_back({column, value});
        } else {
            Counter pos = columns_map_[column];
            if (columns_[pos].second < value)
                columns_[pos].second = value;
        }
    }

private:

    /*
     * Private methods
     */

    /*
     * Private attributes
     */

    /** Columns. */
    std::vector<std::pair<std::shared_ptr<const Column>, Value>> columns_;

    /** Map of columns to position in solution.columns_. */
    std::unordered_map<std::shared_ptr<const Column>, Counter> columns_map_;

};

/**
 * Solution class.
 */
class Solution
{

public:

    /** Get model. */
    const Model& model() const { return *model_; }

    /** Return 'true' iff the solution is feasible. */
    bool feasible() const { return feasible_; }

    /**
     * Return 'true' iff the solution is feasible for the constraints (but not
     * necessarily for the variable integrality).
     */
    bool feasible_relaxation() const { return feasible_relaxation_; }

    /** Get the objective value of the solution. */
    Value objective_value() const { return objective_value_; }

    /** Get columns. */
    const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& columns() const { return columns_; };

    /*
     * Export
     */

    /** Export solution characteristics to a JSON structure. */
    nlohmann::json to_json() const;

    /** Write a formatted output of the instance to a stream. */
    void format(
            std::ostream& os,
            int verbosity_level = 1) const
    {
        double tol = 1e-4;
        if (verbosity_level >= 1) {
            os
                << "Feasible:           " << feasible() << std::endl
                << "Value:              " << objective_value() << std::endl
                << "Number of columns:  " << columns_.size() << std::endl
                ;
        }

        if (verbosity_level >= 2) {
            os << std::right << std::endl
                << std::setw(12) << "Row"
                << std::setw(36) << "Name"
                << std::setw(12) << "Lower"
                << std::setw(12) << "Value"
                << std::setw(12) << "Upper"
                << std::setw(12) << "Feasible"
                << std::endl
                << std::setw(12) << "---"
                << std::setw(36) << "----"
                << std::setw(12) << "-----"
                << std::setw(12) << "-----"
                << std::setw(12) << "-----"
                << std::setw(12) << "--------"
                << std::endl;
            for (RowIdx row_id = 0;
                    row_id < (RowIdx)this->model().rows.size();
                    ++row_id) {
                const Row& row = this->model().rows[row_id];
                bool infeasible = (row_values_[row_id] > model_->rows[row_id].upper_bound + tol)
                    || (row_values_[row_id] < model_->rows[row_id].lower_bound - tol);
                os
                    << std::setw(12) << row_id
                    << std::setw(36) << row.name
                    << std::setw(12) << row.lower_bound
                    << std::setw(12) << row_values_[row_id]
                    << std::setw(12) << row.upper_bound
                    << std::setw(12) << !infeasible
                    << std::endl;
            }

            os
                << std::endl
                << std::setw(12) << "Name"
                << std::setw(12) << "Type"
                << std::setw(12) << "Value"
                << std::setw(12) << "Integral"
                << std::endl
                << std::setw(12) << "----"
                << std::setw(12) << "----"
                << std::setw(12) << "-----"
                << std::setw(12) << "--------"
                << std::endl;
            for (const auto& p: this->columns()) {
                Value value = p.second;
                Value fractionality = std::fabs(value - std::round(value));
                bool integral = (p.first->type == VariableType::Continuous)
                    || !(fractionality > tol);
                os
                    << std::setw(12) << p.first->name
                    << std::setw(12) << ((p.first->type == VariableType::Continuous)? "C": "I")
                    << std::setw(12) << p.second
                    << std::setw(12) << integral
                    << std::endl;
            }
        }
    }

private:

    /** Constructor. */
    Solution() { }

    /** Model. */
    const Model* model_;

    /** Feasible. */
    bool feasible_;

    /**
     * Feasible regarding the constraints but the regarding the column
     * integrality.
     */
    bool feasible_relaxation_;

    /** Objective value. */
    Value objective_value_;

    /** Row values. */
    std::vector<Value> row_values_;

    /** Columns. */
    std::vector<std::pair<std::shared_ptr<const Column>, Value>> columns_;

    friend class SolutionBuilder;

};

class SolutionBuilder
{

public:

    /** Constructor. */
    SolutionBuilder() { }

    /** Set the model of the solution. */
    SolutionBuilder& set_model(const Model& model) { solution_.model_ = &model; return *this; }

    /** Add a column to the solution. */
    void add_column(
            const std::shared_ptr<const Column>& column,
            Value value)
    {
        if (value == 0)
            return;
        if (columns_map_.find(column) == columns_map_.end()) {
            columns_map_[column] = solution_.columns_.size();
            solution_.columns_.push_back({column, value});
        } else {
            Counter pos = columns_map_[column];
            solution_.columns_[pos].second = solution_.columns_[pos].second + value;
        }
    }

    /** Build. */
    Solution build()
    {
        compute_feasible();
        compute_objective_value();

        return std::move(solution_);
    }

private:

    /*
     * Private methods
     */

    /** Compute the feasibility of the solution. */
    void compute_feasible()
    {
        double tol = 1e-4;
        //std::cout << "compute_feasible" << std::endl;
        solution_.row_values_ = std::vector<Value>(solution_.model_->rows.size(), 0.0);
        for (const auto& p: solution_.columns_) {
            const Column& column = *p.first;
            Value column_value = p.second;
            for (const LinearTerm& element: column.elements) {
                solution_.row_values_[element.row] += column_value * element.coefficient;
            }
        }

        solution_.feasible_ = true;
        solution_.feasible_relaxation_ = true;
        for (RowIdx row = 0;
                row < (RowIdx)solution_.model_->rows.size();
                ++row) {
            if (solution_.row_values_[row] > solution_.model_->rows[row].upper_bound + tol) {
                //std::cout << "row " << row
                //    << " name " << solution_.model_->rows[row].name
                //    << " lb " << solution_.model_->rows[row].lower_bound
                //    << " val " << solution_.row_values_[row]
                //    << " ub " << solution_.model_->rows[row].upper_bound
                //    << std::endl;
                solution_.feasible_ = false;
                solution_.feasible_relaxation_ = false;
            }
            if (solution_.row_values_[row] < solution_.model_->rows[row].lower_bound - tol) {
                //std::cout << "row " << row
                //    << " name " << solution_.model_->rows[row].name
                //    << " lb " << solution_.model_->rows[row].lower_bound
                //    << " val " << solution_.row_values_[row]
                //    << " ub " << solution_.model_->rows[row].upper_bound
                //    << std::endl;
                solution_.feasible_ = false;
                solution_.feasible_relaxation_ = false;
            }
        }

        for (const auto& p: solution_.columns_) {
            const Column& column = *p.first;
            Value value = p.second;
            if (column.type == VariableType::Integer) {
                Value fractionality = std::fabs(value - std::round(value));
                if (fractionality > tol) {
                    //std::cout << "column " << column
                    //    << " value " << value
                    //    << " frac " << fractionality
                    //    << std::endl;
                    solution_.feasible_ = false;
                }
            }
        }
    }

    /** Compute the objective value of the solution. */
    void compute_objective_value()
    {
        solution_.objective_value_ = 0.0;
        for (const auto& p: solution_.columns_) {
            const Column& column = *p.first;
            Value column_value = p.second;
            solution_.objective_value_ += column.objective_coefficient * column_value;
        }
    }

    /*
     * Private attributes
     */

    /** Solution. */
    Solution solution_;

    /** Map of columns to position in solution.columns_. */
    std::unordered_map<std::shared_ptr<const Column>, Counter> columns_map_;

};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Implementation ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline Value norm(
        const std::vector<RowIdx>& new_rows,
        const std::vector<Value>& vector)
{
    Value res = 0;
    for (RowIdx row_pos = 0; row_pos < (RowIdx)new_rows.size(); ++row_pos) {
        RowIdx i = new_rows[row_pos];
        res += vector[i] * vector[i];
    }
    return std::sqrt(res);
}

inline Value norm(
        const std::vector<RowIdx>& new_rows,
        const std::vector<Value>& vector_1,
        const std::vector<Value>& vector_2)
{
    Value res = 0;
    for (RowIdx row_pos = 0; row_pos < (RowIdx)new_rows.size(); ++row_pos) {
        RowIdx i = new_rows[row_pos];
        res += (vector_2[i] - vector_1[i]) * (vector_2[i] - vector_1[i]);
    }
    return std::sqrt(res);
}

/**
 * Content hash/equality functor over 'shared_ptr<const Column>': two
 * distinct 'Column' objects with the same objective coefficient and the
 * same multiset of (row, coefficient) elements compare/hash equal --
 * indifferent to how many times, or in what order, a logically-identical
 * column was discovered. Used to recognize when a freshly-priced column
 * is logically the same as one already known (e.g. the same route/pattern
 * rediscovered under different duals).
 */
struct ColumnHasher
{
    std::hash<RowIdx> hasher_row;
    std::hash<Value> hasher_value;
    mutable optimizationtools::IndexedMap<Value> elements_tmp;

    ColumnHasher(const Model& model):
        elements_tmp(model.rows.size(), 0) { }

    inline bool operator()(
            const std::shared_ptr<const Column>& column_1,
            const std::shared_ptr<const Column>& column_2) const
    {
        if (column_1->objective_coefficient
                != column_2->objective_coefficient)
            return false;
        elements_tmp.clear();
        for (const LinearTerm& element: column_1->elements)
            elements_tmp.set(element.row, element.coefficient);
        for (const LinearTerm& element: column_2->elements)
            if (elements_tmp[element.row] != element.coefficient)
                return false;
        elements_tmp.clear();
        for (const LinearTerm& element: column_2->elements)
            elements_tmp.set(element.row, element.coefficient);
        for (const LinearTerm& element: column_1->elements)
            if (elements_tmp[element.row] != element.coefficient)
                return false;
        return true;
    }

    inline std::size_t operator()(
            const std::shared_ptr<const Column>& column) const
    {
        size_t hash = hasher_value(column->objective_coefficient);
        size_t hash_tmp = 0;
        for (const LinearTerm& element: column->elements) {
            size_t hash_tmp_2 = hasher_row(element.row);
            optimizationtools::hash_combine(hash_tmp_2, hasher_value(element.coefficient));
            hash_tmp += hash_tmp_2;
        }
        optimizationtools::hash_combine(hash, hash_tmp);
        return hash;
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct Output: optimizationtools::Output
{
    /** Constructor. */
    Output(const Model& model):
        solution(SolutionBuilder().set_model(model).build()),
        bound((model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                -std::numeric_limits<Value>::infinity():
                +std::numeric_limits<Value>::infinity()),
        relaxation_solution(SolutionBuilder().set_model(model).build()) { }


    /** Solution. */
    Solution solution;

    /** Bound. */
    Value bound;

    /** Elapsed time. */
    double time = 0.0;

    /** Time spent solving the LP subproblems. */
    double time_lpsolve = 0.0;

    /** Time spent solving the pricing subproblems. */
    double time_pricing = 0.0;

    /**
     * Time spent in the rounding heuristic (Phase 1 greedy fixing plus,
     * when entered, Phase 2's fix/price/fix completion loop). Included in,
     * not additional to, 'time'; kept separate from 'time_pricing' since
     * it's a distinct, optional side computation, not part of the regular
     * per-iteration pricing.
     */
    double time_rounding_heuristic = 0.0;

    /** Number of column generation iterations. */
    Counter number_of_column_generation_iterations = 0;

    /** Columns generated during the algorithm. */
    std::vector<std::shared_ptr<const Column>> columns;

    /** Solution. */
    Solution relaxation_solution;


    std::string solution_value() const
    {
        return optimizationtools::solution_value(
                solution.model().objective_sense,
                solution.feasible(),
                solution.objective_value());
    }

    /**
     * String representation of 'bound'. JSON has no representation for
     * +/-inf (nlohmann::json silently serializes both to 'null', making
     * them indistinguishable), so 'bound' is exposed to JSON as a string
     * instead of a raw number, the same way 'solution_value()' already is.
     * This matters in particular because 'bound' reaching +inf
     * (minimization) or -inf (maximization) is how infeasibility is
     * signaled (see 'ColumnGenerationOutput::relaxation_solution_is_feasible').
     */
    std::string bound_string() const
    {
        std::stringstream ss;
        ss << bound;
        return ss.str();
    }

    double absolute_optimality_gap() const
    {
        return optimizationtools::absolute_optimality_gap(
                solution.model().objective_sense,
                solution.feasible(),
                solution.objective_value(),
                bound);
    }

    double relative_optimality_gap() const
    {
       return optimizationtools::relative_optimality_gap(
               solution.model().objective_sense,
               solution.feasible(),
               solution.objective_value(),
               bound);
    }

    /**
     * 'true' iff nothing further the algorithm could find would change
     * the outcome:
     * - With a feasible 'solution': it already matches 'bound', so the
     *   optimality gap is provably zero. Checked one-sided and
     *   sense-aware (not via 'std::abs(objective_value() - bound) <
     *   FFOT_TOL') so that a future bug putting 'bound' on the wrong side
     *   of 'objective_value()' -- which should never happen, by weak
     *   duality -- shows up as this staying 'false' instead of silently
     *   still comparing "close enough".
     * - Without one yet: 'bound' alone has reached the sense-aware
     *   infinity sentinel, proving the model infeasible. Can't use the
     *   same formula as the feasible case here -- 'solution.
     *   objective_value()' reads as a meaningless 0 (not infinity) for
     *   the default/empty solution before any incumbent is found, which
     *   would otherwise make the comparison fire as soon as 'bound'
     *   naturally crosses 0 during ordinary iteration, long before
     *   'bound' actually proves anything. Must check the sense-correct
     *   infinity only, not both signs: the *other* sign is 'bound''s
     *   vacuous default before any real work happens (e.g. -inf for
     *   Minimize), which would otherwise make this true from the very
     *   first check.
     */
    bool optimal() const
    {
        bool minimize = (solution.model().objective_sense == optimizationtools::ObjectiveDirection::Minimize);
        if (!solution.feasible()) {
            return bound == ((minimize)?
                    std::numeric_limits<Value>::infinity():
                    -std::numeric_limits<Value>::infinity());
        }
        return (minimize)?
            (solution.objective_value() <= bound + FFOT_TOL):
            (solution.objective_value() >= bound - FFOT_TOL);
    }

    virtual nlohmann::json to_json() const
    {
        return {
            {"Value", solution_value()},
            {"Bound", bound_string()},
            {"AbsoluteOptimalityGap", absolute_optimality_gap()},
            {"RelativeOptimalityGap", relative_optimality_gap()},
            {"Time", time},
            {"PricingTime", time_pricing},
            {"LpTime", time_lpsolve},
            {"RoundingHeuristicTime", time_rounding_heuristic},
            {"NumberOfColumnGenerationIterations", number_of_column_generation_iterations},
        };
    }

    virtual int format_width() const { return 30; }

    virtual void format(std::ostream& os) const
    {
        int width = format_width();
        os
            << std::setw(width) << std::left << "Value: " << solution_value() << std::endl
            << std::setw(width) << std::left << "Bound: " << bound << std::endl
            << std::setw(width) << std::left << "Absolute optimality gap: " << absolute_optimality_gap() << std::endl
            << std::setw(width) << std::left << "Relative optimality gap (%): " << relative_optimality_gap() * 100 << std::endl
            << std::setw(width) << std::left << "Time: " << time << std::endl
            << std::setw(width) << std::left << "Pricing time: " << time_pricing << std::endl
            << std::setw(width) << std::left << "Linear programming time: " << time_lpsolve << std::endl
            << std::setw(width) << std::left << "Rounding heuristic time: " << time_rounding_heuristic << std::endl
            << std::setw(width) << std::left << "Number of CG iterations: " << number_of_column_generation_iterations << std::endl
            << std::setw(width) << std::left << "Number of new columns: " << columns.size() << std::endl
            ;
    }
};

using NewSolutionCallback = std::function<void(const Output&)>;

enum class SolverName { CLP, Highs, CPLEX, Xpress, Knitro };

inline std::istream& operator>>(
        std::istream& in,
        SolverName& solver_name)
{
    std::string token;
    in >> token;
    if (token == "clp" || token == "CLP") {
        solver_name = SolverName::CLP;
    } else if (token == "highs" || token == "Highs" || token == "HIGHS") {
        solver_name = SolverName::Highs;
    } else if (token == "cplex" || token == "Cplex" || token == "CPLEX") {
        solver_name = SolverName::CPLEX;
    } else if (token == "xpress" || token == "Xpress" || token == "XPRESS") {
        solver_name = SolverName::Xpress;
    } else if (token == "knitro" || token == "Knitro") {
        solver_name = SolverName::Knitro;
    } else  {
        in.setstate(std::ios_base::failbit);
    }
    return in;
}

inline std::ostream& operator<<(
        std::ostream& os,
        SolverName solver_name)
{
    switch (solver_name) {
    case SolverName::CLP: {
        os << "CLP";
        break;
    } case SolverName::Highs: {
        os << "Highs";
        break;
    } case SolverName::CPLEX: {
        os << "CPLEX";
        break;
    } case SolverName::Xpress: {
        os << "Xpress";
        break;
    } case SolverName::Knitro: {
        os << "Knitro";
        break;
    }
    }
    return os;
}

inline SolverName s2lps(const std::string& s)
{
    if (s == "clp" || s == "CLP") {
        return SolverName::CLP;
    } else if (s == "highs" || s == "Highs" || s == "HIGHS") {
        return SolverName::Highs;
    } else if (s == "cplex" || s == "Cplex" || s == "CPLEX") {
        return SolverName::CPLEX;
    } else if (s == "xpress" || s == "Xpress" || s == "XPRESS") {
        return SolverName::Xpress;
    } else if (s == "knitro" || s == "Knitro") {
        return SolverName::Knitro;
    } else {
        return SolverName::CLP;
    }
}

struct Parameters: optimizationtools::Parameters
{
    /** Callback function called when a new best solution is found. */
    NewSolutionCallback new_solution_callback = [](const Output&) { };

    /** Callback function called when a new best bound is found. */
    NewSolutionCallback new_bound_callback = [](const Output&) { };

    /** Column pool. */
    std::vector<std::shared_ptr<const Column>> column_pool;

    /** Initial columns. */
    std::vector<std::shared_ptr<const Column>> initial_columns;

    /** Fixed columns. */
    std::vector<std::pair<std::shared_ptr<const Column>, Value>> fixed_columns;

    /**
     * Branching decisions.
     *
     * Unlike 'column_pool'/'initial_cuts', this is node-local, not a pool:
     * it is expected to hold exactly the decisions on the path from the
     * root to the current node, rebuilt per node (like 'fixed_columns'),
     * never accumulated across the whole search.
     */
    std::vector<std::shared_ptr<const BranchingDecision>> branching_decisions;

    /** Cuts to seed the active cut set with. */
    std::vector<std::shared_ptr<const Cut>> initial_cuts;

    /**
     * Enable internal diving:
     * - 0: not enabled
     * - 1: enabled at the root node
     * - 2: enabled at all nodes
     */
    int internal_diving = 0;

    /**
     * Enable cutting planes:
     * - 0: not enabled
     * - 1: enabled at the root node
     * - 2: enabled at all nodes
     */
    int cutting_planes = 0;

    /**
     * Enable the inline rounding heuristic (run at every column generation
     * iteration; see 'ColumnGenerationParameters::
     * rounding_heuristic_infeasibility_threshold' for its stop condition):
     * - 0: not enabled
     * - 1: enabled at the root node
     * - 2: enabled at all nodes
     */
    int rounding_heuristic = 0;


    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = optimizationtools::Parameters::to_json();
        json.merge_patch({
                {"NumberOfColumnsInTheColumnPool", column_pool.size()},
                {"NumberOfInitialColumns", initial_columns.size()},
                {"NumberOfFixedColumns", fixed_columns.size()},
                {"NumberOfBranchingDecisions", branching_decisions.size()},
                {"NumberOfInitialCuts", initial_cuts.size()},
                {"InternalDiving", internal_diving},
                {"CuttingPlanes", cutting_planes},
                {"RoundingHeuristic", rounding_heuristic},
                });
        return json;
    }

    virtual int format_width() const override { return 41; }

    virtual void format(std::ostream& os) const override
    {
        optimizationtools::Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Number of columns in the column pool: " << column_pool.size() << std::endl
            << std::setw(width) << std::left << "Number of initial columns: " << initial_columns.size() << std::endl
            << std::setw(width) << std::left << "Number of fixed columns: " << fixed_columns.size() << std::endl
            << std::setw(width) << std::left << "Number of branching decisions: " << branching_decisions.size() << std::endl
            << std::setw(width) << std::left << "Number of initial cuts: " << initial_cuts.size() << std::endl
            << std::setw(width) << std::left << "Internal diving: " << internal_diving << std::endl
            << std::setw(width) << std::left << "Cutting planes: " << cutting_planes << std::endl
            << std::setw(width) << std::left << "Rounding heuristic: " << rounding_heuristic << std::endl
            ;
    }
};

}
