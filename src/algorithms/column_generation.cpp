#include "columngenerationsolver/algorithms/column_generation.hpp"

#include "columngenerationsolver/algorithm_formatter.hpp"

#include "linear_programming_solver.hpp"

using namespace columngenerationsolver;

namespace
{

/** Violation of bound ['lower_bound', 'upper_bound'] at 'value'. */
Value rounding_heuristic_violation(
        Value lower_bound,
        Value upper_bound,
        Value value)
{
    return (std::max)(0.0, lower_bound - value) + (std::max)(0.0, value - upper_bound);
}

/**
 * A single row's (or the objective's) contribution to the rounding
 * heuristic's total infeasibility at 'value': the ratio of its current
 * violation to its violation at the start of the call ('violation_start'),
 * or 0 if it was already satisfied then (0 either because it genuinely
 * was, or because there is no incumbent yet to violate) — the greedy
 * fixing the rounding heuristic does never lets a row/the objective become
 * violated once it wasn't at the start.
 */
Value rounding_heuristic_infeasibility_contribution(
        Value lower_bound,
        Value upper_bound,
        Value value,
        Value violation_start)
{
    if (violation_start <= 0.0)
        return 0.0;
    return rounding_heuristic_violation(lower_bound, upper_bound, value) / violation_start;
}

using ColumnPool = std::unordered_set<
    std::shared_ptr<const Column>,
    const ColumnHasher&,
    const ColumnHasher&>;

/**
 * Whether 'value' (sense-aware: a proven bound, or a hypothetical "best
 * case" relaxation value) is already no better than
 * 'parameters.objective_cutoff' -- i.e. whether a node stuck at 'value'
 * could be cut against that cutoff. 'parameters.objective_cutoff' at its
 * default (+infinity) is a sentinel meaning "no cutoff, interested in
 * anything", checked by value-equality rather than as a literal bound, so
 * this always returns 'false' then, regardless of sense.
 */
bool cutoff_reached(
        const Model& model,
        const ColumnGenerationParameters& parameters,
        Value value)
{
    if (parameters.objective_cutoff == std::numeric_limits<Value>::infinity())
        return false;
    return (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
        (value >= parameters.objective_cutoff - FFOT_TOL):
        (value <= parameters.objective_cutoff + FFOT_TOL);
}

/**
 * 'cut''s value at 'relaxation_solution': the sum of the relaxation's own
 * column values weighted by their coefficient in 'cut'.
 */
Value cut_value(
        const Model& model,
        const Cut& cut,
        const Solution& relaxation_solution)
{
    Value value = 0.0;
    for (const auto& p: relaxation_solution.columns())
        value += p.second * model.pricing_solver->coefficient(cut, *p.first);
    return value;
}

/**
 * Whether 'cut' is genuinely violated by 'relaxation_solution': its value
 * falls outside ['lower_bound', 'upper_bound'] by more than its own
 * 'feasibility_tolerance'. The one definition of "violated" used
 * throughout -- for a cut pool candidate before reactivating it, and for
 * whatever 'PricingSolver::separate_cuts' returns before it's ever
 * trusted -- a pluggable interface's own notion of "violated" is never
 * taken at face value.
 */
bool cut_is_violated(
        const Model& model,
        const Cut& cut,
        const Solution& relaxation_solution)
{
    Value value = cut_value(model, cut, relaxation_solution);
    return value < cut.lower_bound - cut.feasibility_tolerance
        || value > cut.upper_bound + cut.feasibility_tolerance;
}

/**
 * How far 'cut''s value at 'relaxation_solution' falls outside
 * ['lower_bound', 'upper_bound'] -- 0 if it's satisfied (even if only
 * barely, unlike 'cut_is_violated' this doesn't account for
 * 'feasibility_tolerance'). Used to rank violated cuts against each
 * other and activate only the most violated one per round, not to decide
 * whether a cut is violated at all.
 */
Value cut_violation(
        const Model& model,
        const Cut& cut,
        const Solution& relaxation_solution)
{
    Value value = cut_value(model, cut, relaxation_solution);
    return (std::max)(0.0, cut.lower_bound - value) + (std::max)(0.0, value - cut.upper_bound);
}

/**
 * Input for 'run_column_generation_attempt()': one "build a master LP from
 * a given column set/weighting and run column generation to convergence"
 * attempt, matching what the dummy-column retry loop used to inline
 * directly. Plain data only, no behavior (see the free function further
 * below) — same rationale as 'RoundingHeuristicInput' below: too much
 * state for a plain parameter list to stay readable.
 */
struct ColumnGenerationAttemptInput
{
    const Model& model;
    const ColumnGenerationParameters& parameters;
    RowIdx number_of_rows;
    const std::vector<Value>& row_values;
    Value c0;
    const std::vector<RowIdx>& new_row_indices;
    const std::vector<RowIdx>& new_rows;
    RowIdx new_number_of_rows;
    const std::vector<Value>& new_row_lower_bounds;
    const std::vector<Value>& new_row_upper_bounds;
    // Paired with the relaxation value at the point each cut joined this
    // vector (see 'column_generation()''s own 'solver_cuts' for how that's
    // used to decide when a cut is stale enough to remove).
    const std::vector<std::pair<std::shared_ptr<const Cut>, Value>>& solver_cuts;
    const std::vector<Value>& new_cut_lower_bounds;
    const std::vector<Value>& new_cut_upper_bounds;
    const std::vector<std::shared_ptr<const Column>>& initial_columns;

    /**
     * 'true' for the feasibility phase: dummy columns get a fixed weight
     * of 1, every other column (static, initial, newly-priced) gets its
     * objective coefficient forced to 0, and pricing is called with
     * 'solve_feasibility=true'. 'false' for the optimality phase: no
     * dummy columns at all, real objective coefficients throughout.
     */
    bool solve_feasibility;

    /**
     * Which cutting-plane round this attempt belongs to (0 for the very
     * first one, before any cut has been added). Used to restrict
     * 'ColumnGenerationParameters::internal_diving' at level 1 to that
     * very first round -- see 'run_column_generation_attempt'.
     */
    Counter cutting_plane_iteration;

    /**
     * Budget, shared across every phase and cutting-plane round of the
     * whole 'column_generation()' call (not just this one attempt), of how
     * many more times 'solve_pricing' may still be called with
     * 'PricingSolver::PricingType::Dual' this call -- see
     * 'run_column_generation_attempt'. Capped at 2 (see 'PricingSolver::
     * solve_pricing').
     */
    Counter& number_of_dual_pricing_calls;

    /**
     * 'output.relaxation_solution_value' as it stood at the end of the
     * previous cutting-plane iteration (optimality phase), or an
     * unreachable sentinel (+inf minimizing, -inf maximizing) for the very
     * first iteration, when there is no previous round to compare against.
     * Cutting planes only ever shrink the relaxation's feasible region, so
     * this value can never get worse from one cutting-plane iteration to
     * the next -- once the current attempt's relaxation, mid-convergence,
     * already matches or beats it, further 'solve_pricing' calls this
     * round would only spend time re-deriving a bound this round was
     * already guaranteed to reach, before the next round's cut narrows
     * things further anyway. Only meaningful for the optimality phase: the
     * feasibility phase's LP value is a different, non-comparable
     * objective (see 'solve_feasibility').
     */
    Value previous_cutting_plane_relaxation_value;

    // Mutated across the whole 'column_generation()' call, not just this
    // one attempt.
    ColumnPool& column_pool;
    ColumnGenerationOutput& output;
    AlgorithmFormatter& algorithm_formatter;
};

/**
 * Input/state for the inline rounding heuristic (see
 * 'ColumnGenerationParameters::rounding_heuristic'): the pieces of
 * 'column_generation()'s state it needs to read and/or mutate, bundled
 * here since there are too many for a plain parameter list to stay
 * readable — plain data only, no behavior (see the free functions below).
 *
 * Built once (references stay valid: the referenced containers get
 * mutated in place across iterations, never reallocated to a new object),
 * then 'run_rounding_heuristic()' is called every iteration. Never
 * touches the master LP, 'solver_cuts', or the real
 * 'parameters.fixed_columns' — it's a side computation, working on its
 * own local copies of 'row_values'/'c0' — except for 'column_pool'/
 * 'output.columns', which it updates the same way regular pricing
 * discoveries do, so newly found columns are available for reuse by the
 * real pricing loop too.
 */
struct RoundingHeuristicInput
{
    // Shares the read-only real column generation state, plus
    // 'column_pool'/'output'/'algorithm_formatter', with the enclosing
    // 'run_column_generation_attempt()' call.
    ColumnGenerationAttemptInput& attempt_input;

    LinearProgrammingSolver* solver;
    const std::vector<std::shared_ptr<const Column>>& solver_columns;
    const std::vector<Value>& duals_out;
    const std::vector<std::pair<std::shared_ptr<const Cut>, Value>>& cut_duals;

    // Set fresh at the start of every 'run_rounding_heuristic()' call,
    // from 'output.solution' as it stands then; read by
    // 'rounding_heuristic_max_value'/'rounding_heuristic_fix_column'.
    bool has_incumbent = false;
    Value objective_lower_bound = 0.0;
    Value objective_upper_bound = 0.0;
    std::vector<Value> row_violation_start;
    Value objective_violation_start = 0.0;
};

/**
 * Highest value 'column' can be fixed to (rounded down for integer
 * columns) without increasing any row's or the objective's infeasibility,
 * given the diving state so far.
 */
Value rounding_heuristic_max_value(
        const RoundingHeuristicInput& input,
        const std::shared_ptr<const Column>& column,
        const std::vector<Value>& row_values_tmp,
        Value c0_tmp)
{
    Value value = column->upper_bound;
    for (const LinearTerm& element: column->elements) {
        if (element.coefficient > 0) {
            Value v = (input.attempt_input.model.rows[element.row].upper_bound - row_values_tmp[element.row])
                / element.coefficient;
            value = (std::min)(value, v);
        } else if (element.coefficient < 0) {
            Value v = (row_values_tmp[element.row] - input.attempt_input.model.rows[element.row].lower_bound)
                / (-element.coefficient);
            value = (std::min)(value, v);
        }
    }
    if (input.has_incumbent) {
        if (column->objective_coefficient > 0
                && input.objective_upper_bound != std::numeric_limits<Value>::infinity()) {
            Value v = (input.objective_upper_bound - c0_tmp) / column->objective_coefficient;
            value = (std::min)(value, v);
        } else if (column->objective_coefficient < 0
                && input.objective_lower_bound != -std::numeric_limits<Value>::infinity()) {
            Value v = (input.objective_lower_bound - c0_tmp) / column->objective_coefficient;
            value = (std::min)(value, v);
        }
    }
    if (column->type == VariableType::Integer)
        value = std::floor(value);
    return (std::max)(0.0, value);
}

/**
 * Fix 'column' at 'value', updating 'row_values_tmp'/'c0_tmp' and the
 * running 'infeasibility' incrementally — only the rows 'column' touches,
 * plus the objective, rather than recomputing the sum over every row.
 */
void rounding_heuristic_fix_column(
        const RoundingHeuristicInput& input,
        const std::shared_ptr<const Column>& column,
        Value value,
        std::vector<Value>& row_values_tmp,
        Value& c0_tmp,
        Value& infeasibility)
{
    for (const LinearTerm& element: column->elements) {
        infeasibility -= rounding_heuristic_infeasibility_contribution(
                input.attempt_input.model.rows[element.row].lower_bound,
                input.attempt_input.model.rows[element.row].upper_bound,
                row_values_tmp[element.row],
                input.row_violation_start[element.row]);
        row_values_tmp[element.row] += value * element.coefficient;
        infeasibility += rounding_heuristic_infeasibility_contribution(
                input.attempt_input.model.rows[element.row].lower_bound,
                input.attempt_input.model.rows[element.row].upper_bound,
                row_values_tmp[element.row],
                input.row_violation_start[element.row]);
    }
    infeasibility -= rounding_heuristic_infeasibility_contribution(
            input.objective_lower_bound,
            input.objective_upper_bound,
            c0_tmp,
            input.objective_violation_start);
    c0_tmp += value * column->objective_coefficient;
    infeasibility += rounding_heuristic_infeasibility_contribution(
            input.objective_lower_bound,
            input.objective_upper_bound,
            c0_tmp,
            input.objective_violation_start);
}

/**
 * Make a newly pricing-discovered column available for reuse by the real
 * pricing loop too, exactly like the "Store these new columns" step in
 * 'column_generation()' does for regular pricing discoveries.
 */
void rounding_heuristic_add_to_column_pool(
        RoundingHeuristicInput& input,
        const std::shared_ptr<const Column>& column)
{
    if (input.attempt_input.column_pool.find(column) != input.attempt_input.column_pool.end())
        return;
    input.attempt_input.column_pool.insert(column);
    input.attempt_input.output.columns.push_back(column);
}

void run_rounding_heuristic(RoundingHeuristicInput& input)
{
    auto start = std::chrono::high_resolution_clock::now();

    bool minimize = (input.attempt_input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize);
    input.has_incumbent = input.attempt_input.output.solution.feasible();
    Value incumbent = (input.has_incumbent)? input.attempt_input.output.solution.objective_value(): 0.0;
    input.objective_lower_bound = (minimize)?
        -std::numeric_limits<Value>::infinity():
        incumbent;
    input.objective_upper_bound = (minimize)?
        incumbent:
        std::numeric_limits<Value>::infinity();

    // Each row's (and, once there's an incumbent, the objective's)
    // violation at the start of this call — the normalizing denominator
    // 'rounding_heuristic_infeasibility_contribution' uses, and the fixed
    // reference point that makes "0 if it was already satisfied then"
    // meaningful. Computed once per call, not per column fixed; 0 when
    // there's no incumbent, which then makes every
    // 'rounding_heuristic_infeasibility_contribution' call for the
    // objective return 0 regardless of 'value', so call sites don't need
    // their own 'has_incumbent' check.
    input.row_violation_start.assign(input.attempt_input.number_of_rows, 0.0);
    for (RowIdx row_id = 0; row_id < input.attempt_input.number_of_rows; ++row_id) {
        input.row_violation_start[row_id] = rounding_heuristic_violation(
                input.attempt_input.model.rows[row_id].lower_bound,
                input.attempt_input.model.rows[row_id].upper_bound,
                input.attempt_input.row_values[row_id]);
    }
    input.objective_violation_start = (input.has_incumbent)?
        rounding_heuristic_violation(input.objective_lower_bound, input.objective_upper_bound, input.attempt_input.c0):
        0.0;

    Value initial_infeasibility = 0.0;
    for (RowIdx row_id = 0; row_id < input.attempt_input.number_of_rows; ++row_id) {
        initial_infeasibility += rounding_heuristic_infeasibility_contribution(
                input.attempt_input.model.rows[row_id].lower_bound,
                input.attempt_input.model.rows[row_id].upper_bound,
                input.attempt_input.row_values[row_id],
                input.row_violation_start[row_id]);
    }
    initial_infeasibility += rounding_heuristic_infeasibility_contribution(
            input.objective_lower_bound,
            input.objective_upper_bound,
            input.attempt_input.c0,
            input.objective_violation_start);

    if (initial_infeasibility > 0.0) {
        std::vector<Value> rh_row_values = input.attempt_input.row_values;
        Value rh_c0 = input.attempt_input.c0;
        std::vector<std::pair<std::shared_ptr<const Column>, Value>> fixed_columns;

        // Phase 1: greedily fix the current relaxation's own columns, by
        // decreasing value. No relaxation re-solve, no pricing call.
        std::vector<std::pair<std::shared_ptr<const Column>, Value>> relaxation_columns;
        for (ColIdx column_id = 0;
                column_id < (ColIdx)input.solver_columns.size();
                ++column_id) {
            if (input.solver_columns[column_id] == nullptr)
                continue;
            Value v = input.solver->primal(column_id);
            if (std::abs(v) < FFOT_TOL)
                continue;
            relaxation_columns.push_back({input.solver_columns[column_id], v});
        }
        std::sort(
                relaxation_columns.begin(),
                relaxation_columns.end(),
                [](
                    const std::pair<std::shared_ptr<const Column>, Value>& p1,
                    const std::pair<std::shared_ptr<const Column>, Value>& p2)
                {
                    return p1.second > p2.second;
                });

        Value infeasibility = initial_infeasibility;
        bool threshold_reached = false;
        // Estimate how many more columns would still be needed to resolve
        // the remaining infeasibility, assuming further columns resolve
        // about as much infeasibility each as the last 'rate_window_size'
        // did - not the average since the start of Phase 1.
        // 'relaxation_columns' is sorted by decreasing value, so the first
        // columns fixed are the most useful ones and resolve much more
        // infeasibility each than the ones fixed later; a cumulative
        // average stays optimistic long after the marginal rate has
        // actually collapsed, understating how many columns are really
        // still needed. Recomputing the rate over a small trailing window
        // instead tracks the current, not historical, marginal cost.
        const ColIdx rate_window_size = 10;
        Value window_start_infeasibility = initial_infeasibility;
        ColIdx window_start_columns = 0;
        for (const auto& p: relaxation_columns) {
            Value value = rounding_heuristic_max_value(input, p.first, rh_row_values, rh_c0);
            if (value <= 0.0)
                continue;
            rounding_heuristic_fix_column(input, p.first, value, rh_row_values, rh_c0, infeasibility);
            fixed_columns.push_back({p.first, value});

            ColIdx window_columns = (ColIdx)fixed_columns.size() - window_start_columns;
            Value window_infeasibility_resolved = window_start_infeasibility - infeasibility;
            if (window_infeasibility_resolved > 0.0) {
                Value average_infeasibility_per_column = window_infeasibility_resolved / window_columns;
                Value estimated_remaining_columns = infeasibility / average_infeasibility_per_column;
                if (estimated_remaining_columns < 2.0) {
                    threshold_reached = true;
                    break;
                }
            }
            if (window_columns >= rate_window_size) {
                window_start_infeasibility = infeasibility;
                window_start_columns = (ColIdx)fixed_columns.size();
            }
        }
        // Phase 1 has fixed every relaxation column it had to offer (no
        // more candidates left) but infeasibility remains: there is
        // nothing more this phase can do on its own, so fall through to
        // Phase 2 regardless of the estimate above - completing the
        // solution from here on requires pricing new columns.
        if (!threshold_reached && infeasibility > 0.0)
            threshold_reached = true;

        // Phase 2: complete the solution with a fix/price/fix loop, no
        // relaxation re-solve (mirrors the 'internal_diving' completion
        // loop in 'column_generation()'), only entered once Phase 1
        // resolved enough infeasibility (or ran out of its own columns
        // while infeasibility remained).
        if (threshold_reached) {
            if (infeasibility > 0.0) {
                // Use the real current-iteration duals: they carry the
                // master LP's actual price signal, so pricing keeps
                // proposing columns that are genuinely attractive for the
                // relaxation, not just for the rows still short after
                // Phase 1's greedy fixing.
                for (;;) {
                    auto pricing_output = input.attempt_input.model.pricing_solver->solve_pricing(
                            false,
                            fixed_columns,
                            input.attempt_input.parameters.branching_decisions,
                            input.attempt_input.parameters.tabu,
                            input.duals_out,
                            input.cut_duals,
                            PricingSolver::PricingType::Primal);
                    std::vector<std::shared_ptr<const Column>> new_columns;
                    for (const auto& column: pricing_output.columns) {
                        if (column->elements.empty())
                            continue;
                        // Pooling a tabu column is harmless -- the pool is
                        // just a cache of discovered columns, reusable
                        // later once it isn't tabu, and doesn't by itself
                        // add anything to any LP -- so do it unconditionally.
                        rounding_heuristic_add_to_column_pool(input, column);
                        // Passing 'tabu' to 'solve_pricing' above only lets
                        // the pricing solver *choose* to avoid these
                        // columns -- it isn't required to, so still filter
                        // defensively here: this heuristic's own greedy
                        // fixing has no other check keeping a
                        // branching-excluded column out of the candidate
                        // solution it builds.
                        if (input.attempt_input.parameters.tabu.find(column) != input.attempt_input.parameters.tabu.end())
                            continue;
                        new_columns.push_back(column);
                    }
                    if (new_columns.empty())
                        break;

                    // Sort new columns by reduced cost, using the same
                    // real duals.
                    std::sort(
                            new_columns.begin(),
                            new_columns.end(),
                            [&input, minimize](
                                const std::shared_ptr<const Column>& column_1,
                                const std::shared_ptr<const Column>& column_2)
                            {
                                Value rc1 = input.attempt_input.model.compute_reduced_cost(false, *column_1, input.duals_out, input.cut_duals);
                                Value rc2 = input.attempt_input.model.compute_reduced_cost(false, *column_2, input.duals_out, input.cut_duals);
                                return (minimize)? (rc1 < rc2): (rc1 > rc2);
                            });

                    bool has_fixed = false;
                    for (const auto& column: new_columns) {
                        Value value = rounding_heuristic_max_value(input, column, rh_row_values, rh_c0);
                        if (value <= 0.0)
                            continue;
                        rounding_heuristic_fix_column(input, column, value, rh_row_values, rh_c0, infeasibility);
                        fixed_columns.push_back({column, value});
                        has_fixed = true;

                        if (infeasibility <= 0.0)
                            break;
                    }
                    if (!has_fixed || infeasibility <= 0.0)
                        break;
                }
            }

            // Build and check the candidate solution. Always done (rather
            // than gated on 'infeasibility <= 0.0') so 'Solution::
            // feasible()' — not this heuristic's own approximate
            // ratio-sum metric — is the sole authority on whether it's
            // reported.
            SolutionBuilder solution_builder;
            solution_builder.set_model(input.attempt_input.model);
            for (const auto& p: input.attempt_input.parameters.fixed_columns)
                solution_builder.add_column(p.first, p.second);
            for (const auto& p: fixed_columns)
                solution_builder.add_column(p.first, p.second);
            Solution solution = solution_builder.build();
            if (solution.feasible())
                input.attempt_input.algorithm_formatter.update_solution(solution);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    input.attempt_input.output.time_rounding_heuristic += time_span.count();
}

/**
 * Result of one 'run_column_generation_attempt()' call.
 */
struct ColumnGenerationAttemptResult
{
    /**
     * 'true' iff the relaxation this attempt converged to still needs
     * dummy columns to be feasible. Under 'ColumnGenerationAttemptInput::
     * solve_feasibility', a rigorous infeasibility proof (when the
     * pricing solver's 'overcost' allows one) has already been reported
     * through 'ColumnGenerationAttemptInput::algorithm_formatter' by the
     * time this is 'true' -- the caller only needs this flag to decide
     * whether to fall through to the optimality phase.
     */
    bool has_dummy_column = false;

    /**
     * 'true' iff the timer or the iteration limit fired mid-attempt: the
     * caller must stop immediately (finish up and return from
     * 'column_generation()') rather than act on 'has_dummy_column' or
     * retry.
     */
    bool stop_now = false;
};

/**
 * Build a master LP from 'input.initial_columns' (plus static/fixed
 * columns and dummy columns for any row/cut fixed/initial columns alone
 * can't satisfy), run column generation on it to convergence, and report
 * whether the result still needs dummy columns. Extracted verbatim from
 * what the dummy-column retry loop in 'column_generation()' used to do
 * inline; the retry loop itself (the magnitude-based infeasibility check,
 * the escalate-and-retry logic) stays in 'column_generation()' and calls
 * this once per retry.
 */
ColumnGenerationAttemptResult run_column_generation_attempt(
        ColumnGenerationAttemptInput& input)
{
    ColumnGenerationAttemptResult result;

    // Bound on the best possible reduced cost over the whole attempt, per
    // the 3-way contract on 'PricingOutput::overcost' (exact /
    // heuristic-no-bound / heuristic-with-a-bound). Stays at its
    // sense-aware infinity default until the first pricing call sets it;
    // from then on reflects the last pricing call's value. Used both to
    // stream the tightest bound achievable each iteration (below) and, if
    // 'input.solve_feasibility' and dummy columns persist once this
    // attempt converges, to test for a rigorous infeasibility proof.
    Value overcost = (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
        -std::numeric_limits<Value>::infinity():
        +std::numeric_limits<Value>::infinity();

    // Appends the coefficients of 'column' in the active cuts to 'ri'/'rc',
    // at row indices following the model rows.
    auto append_cut_coefficients = [&input](
            const Column& column,
            std::vector<RowIdx>& ri,
            std::vector<Value>& rc)
    {
        for (CutIdx cut_pos = 0; cut_pos < (CutIdx)input.solver_cuts.size(); ++cut_pos) {
            Value coef = input.model.pricing_solver->coefficient(*input.solver_cuts[cut_pos].first, column);
            if (coef != 0.0) {
                ri.push_back(input.new_number_of_rows + cut_pos);
                rc.push_back(coef);
            }
        }
    };

    // Initialize solver
    //std::cout << "Initialize solver... " << input.parameters.solver_name << std::endl;
    std::vector<Value> lp_row_lower_bounds = input.new_row_lower_bounds;
    std::vector<Value> lp_row_upper_bounds = input.new_row_upper_bounds;
    lp_row_lower_bounds.insert(
            lp_row_lower_bounds.end(),
            input.new_cut_lower_bounds.begin(),
            input.new_cut_lower_bounds.end());
    lp_row_upper_bounds.insert(
            lp_row_upper_bounds.end(),
            input.new_cut_upper_bounds.begin(),
            input.new_cut_upper_bounds.end());

    std::unique_ptr<LinearProgrammingSolver> solver = make_linear_programming_solver(
            input.parameters.solver_name,
            input.model.objective_sense,
            lp_row_lower_bounds,
            lp_row_upper_bounds);

    // This array is used to retrieve the corresponding column from a
    // variable id in the LP solver solution.
    std::vector<std::shared_ptr<const Column>> solver_columns;

    // We never add a generated column more than once in the LP solver.
    // We use this set to keep track of the generated columns inside the
    // LP solver.
    std::unordered_set<std::shared_ptr<const Column>> solver_generated_columns;

    // Verify that 'nonzero_real_columns' -- with no dummy columns -- can
    // still satisfy every row on their own: builds a small LP restricted
    // to exactly those columns (their own bounds, zero objective -- only
    // feasibility matters) and solves it. Used to double-check every
    // magnitude-based "no dummy column" verdict below (there are two:
    // the zero-pricing-calls short circuit and the final relaxation
    // check) -- a dummy column's primal value can sit just under the
    // magnitude tolerance while still being the only thing letting a row
    // be satisfied by the real columns currently available, and handing
    // such a false "dummy-free" verdict to Phase 2 (no dummy columns at
    // all) would make Phase 2's very first solve infeasible, with no
    // duals left to price against.
    auto verify_dummy_free = [&input, &lp_row_lower_bounds, &lp_row_upper_bounds,
         &append_cut_coefficients, &solver_generated_columns](
            const std::vector<std::shared_ptr<const Column>>& nonzero_real_columns) -> bool
    {
        auto start_dummy_free_verification = std::chrono::high_resolution_clock::now();
        std::unique_ptr<LinearProgrammingSolver> verification_solver = make_linear_programming_solver(
                input.parameters.solver_name,
                input.model.objective_sense,
                lp_row_lower_bounds,
                lp_row_upper_bounds);
        for (const std::shared_ptr<const Column>& column: nonzero_real_columns) {
            std::vector<RowIdx> ri;
            std::vector<Value> rc;
            for (const LinearTerm& element: column->elements) {
                ri.push_back(input.new_row_indices[element.row]);
                rc.push_back(element.coefficient);
            }
            append_cut_coefficients(*column, ri, rc);
            // Static columns keep their own bounds; generated (initial or
            // priced) columns are added with [0, +inf), matching how they
            // were originally added to the main LP above.
            bool is_static = (solver_generated_columns.find(column) == solver_generated_columns.end());
            verification_solver->add_column(
                    ri,
                    rc,
                    0,
                    is_static? column->lower_bound: 0.0,
                    is_static? column->upper_bound: std::numeric_limits<Value>::infinity());
        }
        verification_solver->solve();
        bool ok = !verification_solver->infeasible();
        auto end_dummy_free_verification = std::chrono::high_resolution_clock::now();
        input.output.time_dummy_free_verification += std::chrono::duration_cast<std::chrono::duration<double>>(
                end_dummy_free_verification - start_dummy_free_verification).count();
        return ok;
    };

    auto start_lp_construction = std::chrono::high_resolution_clock::now();

    input.output.number_of_columns_in_linear_subproblem = 0;

    // 'infeasible_columns' can only rule out columns it's actually shown
    // -- static columns plus whatever generated columns are being warm-
    // started into this attempt, the same two sets 'infeasible_columns'
    // below is checked against.
    std::vector<std::shared_ptr<const Column>> candidate_columns = input.model.static_columns;
    candidate_columns.insert(
            candidate_columns.end(),
            input.initial_columns.begin(),
            input.initial_columns.end());
    std::vector<std::shared_ptr<const Column>> infeasible_columns
        = input.model.pricing_solver->infeasible_columns(candidate_columns, input.parameters.fixed_columns, input.parameters.branching_decisions);
    std::vector<int8_t> feasible(input.model.static_columns.size(), 1);

    // Add dummy columns. Phase 2 (optimality) has none at all: it is only
    // ever reached once Phase 1 has already found a dummy-free relaxation,
    // so 'has_dummy_column' must stay 'false' by construction.
    std::vector<RowIdx> dummy_column_rows;
    if (input.solve_feasibility) {
        for (RowIdx row_id = 0; row_id < input.new_number_of_rows; ++row_id) {
            if (input.new_row_lower_bounds[row_id] > 0) {
                solver_columns.push_back(nullptr);
                solver->add_column(
                        {row_id},
                        {input.new_row_lower_bounds[row_id]},
                        (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        +1:
                        -1,
                        0,
                        std::numeric_limits<Value>::infinity());
                input.output.number_of_columns_in_linear_subproblem++;
                dummy_column_rows.push_back(row_id);
            }
            if (input.new_row_upper_bounds[row_id] < 0) {
                solver_columns.push_back(nullptr);
                solver->add_column(
                        {row_id},
                        {input.new_row_upper_bounds[row_id]},
                        (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        +1:
                        -1,
                        0,
                        std::numeric_limits<Value>::infinity());
                input.output.number_of_columns_in_linear_subproblem++;
                dummy_column_rows.push_back(row_id);
            }
        }
        // Add dummy columns for cut rows that fixed/static/initial columns
        // alone cannot satisfy (symmetric to the input.model-row dummy
        // columns above).
        for (CutIdx cut_pos = 0; cut_pos < (CutIdx)input.solver_cuts.size(); ++cut_pos) {
            RowIdx cut_row_id = input.new_number_of_rows + cut_pos;
            if (input.new_cut_lower_bounds[cut_pos] > 0) {
                solver_columns.push_back(nullptr);
                solver->add_column(
                        {cut_row_id},
                        {input.new_cut_lower_bounds[cut_pos]},
                        (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        +1:
                        -1,
                        0,
                        std::numeric_limits<Value>::infinity());
                input.output.number_of_columns_in_linear_subproblem++;
                dummy_column_rows.push_back(cut_row_id);
            }
            if (input.new_cut_upper_bounds[cut_pos] < 0) {
                solver_columns.push_back(nullptr);
                solver->add_column(
                        {cut_row_id},
                        {input.new_cut_upper_bounds[cut_pos]},
                        (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        +1:
                        -1,
                        0,
                        std::numeric_limits<Value>::infinity());
                input.output.number_of_columns_in_linear_subproblem++;
                dummy_column_rows.push_back(cut_row_id);
            }
        }
    }

    // Add input.model columns.
    std::vector<Value> lower_bounds;
    std::vector<Value> upper_bounds;
    std::vector<Value> objective_coefficients;
    std::vector<std::vector<RowIdx>> row_ids;
    std::vector<std::vector<Value>> row_coefficients;
    for (const std::shared_ptr<const Column>& column: input.model.static_columns) {
        input.model.check_column(column);

        // Don't add the column if it has already been fixed.
        bool is_fixed = false;
        for (const auto& p: input.parameters.fixed_columns)
            if (p.first.get() == column.get())
                is_fixed = true;
        if (is_fixed)
            continue;

        // Check column feasibility.
        if (std::find(infeasible_columns.begin(), infeasible_columns.end(), column)
                != infeasible_columns.end())
            continue;

        std::vector<RowIdx> ri;
        std::vector<Value> rc;
        bool ok = true;
        //bool print = false;
        //for (const LinearTerm& element: column->elements)
        //    if (element.row == 11380)
        //        print = true;
        for (const LinearTerm& element: column->elements) {
            // The column might not be feasible.
            // For example, it corresponds to the same bin / machine that a
            // currently fixed column or it contains an item / job also
            // included in a currently fixed column.
            if (input.model.rows[element.row].coefficient_lower_bound >= 0
                    && column->type == VariableType::Integer
                    && input.row_values[element.row] + element.coefficient
                    > input.model.rows[element.row].upper_bound) {
                //if (print) {
                //    std::cout << "element " << element.row
                //        << " " << element.coefficient
                //        << std::endl;
                //}
                ok = false;
                break;
            }
            if (input.new_row_indices[element.row] < 0) {
                ok = false;
                break;
            }
            ri.push_back(input.new_row_indices[element.row]);
            rc.push_back(element.coefficient);
        }
        //if (print) {
        //    std::cout << *column << std::endl;
        //    std::cout << "ok " << ok << std::endl;
        //}
        if (!ok)
            continue;
        append_cut_coefficients(*column, ri, rc);
        solver_columns.push_back(column);
        lower_bounds.push_back(column->lower_bound);
        upper_bounds.push_back(column->upper_bound);
        objective_coefficients.push_back(
                input.solve_feasibility? 0: column->objective_coefficient);
        row_ids.push_back(ri);
        row_coefficients.push_back(rc);
        input.output.number_of_columns_in_linear_subproblem++;
    }
    solver->add_columns(
            row_ids,
            row_coefficients,
            objective_coefficients,
            lower_bounds,
            upper_bounds);

    // Add initial columns.
    for (const std::shared_ptr<const Column>& column: input.initial_columns) {
        input.model.check_generated_column(column);

        // Check column feasibility.
        if (std::find(infeasible_columns.begin(), infeasible_columns.end(), column)
                != infeasible_columns.end())
            continue;

        // Don't add a tabu column.
        if (input.parameters.tabu.find(column) != input.parameters.tabu.end())
            continue;

        std::vector<RowIdx> row_ids;
        std::vector<Value> row_coefficients;
        bool ok = true;
        for (const LinearTerm& element: column->elements) {
            // The column might not be feasible.
            // For example, it corresponds to the same bin / machine that a
            // currently fixed column or it contains an item / job also
            // included in a currently fixed column.
            if (input.model.rows[element.row].coefficient_lower_bound >= 0
                    && column->type == VariableType::Integer
                    && input.row_values[element.row] + element.coefficient
                    > input.model.rows[element.row].upper_bound) {
                ok = false;
                break;
            }
            if (input.new_row_indices[element.row] < 0) {
                ok = false;
                break;
            }
            row_ids.push_back(input.new_row_indices[element.row]);
            row_coefficients.push_back(element.coefficient);
        }
        if (!ok)
            continue;
        append_cut_coefficients(*column, row_ids, row_coefficients);
        solver_columns.push_back(column);
        solver_generated_columns.insert(column);
        solver->add_column(
                row_ids,
                row_coefficients,
                input.solve_feasibility? 0: column->objective_coefficient,
                0,
                std::numeric_limits<Value>::infinity());
        input.output.number_of_columns_in_linear_subproblem++;
    }

    auto end_lp_construction = std::chrono::high_resolution_clock::now();
    input.output.time_lp_construction += std::chrono::duration_cast<std::chrono::duration<double>>(
            end_lp_construction - start_lp_construction).count();

    // Duals given to the pricing solver.
    std::vector<Value> duals_sep(input.number_of_rows, 0);
    // π_in, duals at the previous point.
    std::vector<Value> duals_in(input.number_of_rows, 0);
    // π_out, duals of next point without stabilization.
    std::vector<Value> duals_out(input.number_of_rows, 0);
    // π_in + (1 − α) (π_out − π_in)
    std::vector<Value> duals_tilde(input.number_of_rows, 0);
    // Duals in the direction of the subgradient.
    std::vector<Value> duals_g(input.number_of_rows, 0);
    // β π_g + (1 − β) π_out
    std::vector<Value> rho(input.number_of_rows, 0);
    std::vector<Value> lagrangian_constraint_values(input.number_of_rows, 0);
    // g_in.
    std::vector<Value> subgradient(input.number_of_rows, 0);
    // Cut duals (not stabilized: the active cut set is fixed for the
    // whole CG loop, so there is no smoothing history to maintain).
    // Paired with the cut itself (like 'fixed_columns' already is),
    // so 'solve_pricing'/'compute_reduced_cost' callers don't have to
    // separately track 'input.solver_cuts' just to correlate the two.
    std::vector<std::pair<std::shared_ptr<const Cut>, Value>> cut_duals;
    cut_duals.reserve(input.solver_cuts.size());
    for (const auto& p: input.solver_cuts)
        cut_duals.push_back({p.first, 0.0});
    double alpha = input.parameters.static_wentges_smoothing_parameter;

    RoundingHeuristicInput rounding_heuristic_input{
            input,
            solver.get(),
            solver_columns,
            duals_out,
            cut_duals};

    // Whether the previous iteration actually had to call the pricing
    // solver to find new columns (as opposed to finding enough from the
    // column pool alone, without any dual-informed search). Gates the
    // rounding heuristic below: when the pool has been keeping the master
    // fed, the pool's columns were themselves priced against duals close
    // to the current ones, so there is nothing genuinely new for the
    // heuristic's own pricing calls to find yet, and running it would
    // just pay for repeated, redundant pricing work at every iteration.
    // Initialized 'true' so the heuristic still gets a chance to run at
    // the first opportunity.
    bool pricing_called_previous_iteration = true;

    // Feed a pricing call's 'overcost' (see the 3-way contract on
    // 'PricingSolver::PricingOutput::overcost') into 'input.output.bound',
    // exactly the way the per-iteration pricing call below does. Extracted
    // so the dual-oriented escalation further down -- once the primal-
    // oriented loop has converged -- can apply its own call's 'overcost'
    // the same way, without duplicating the infeasibility-proof logic.
    auto apply_overcost = [&input, &solver](Value overcost)
    {
        if (overcost == std::numeric_limits<Value>::infinity())
            return;
        if (!input.solve_feasibility) {
            // Update bound, now that 'overcost' reflects a reduced cost
            // computed at the same duals as 'input.output.
            // relaxation_solution_value' -- giving the tightest bound
            // achievable from the master solve it was computed against.
            Value bound = input.output.relaxation_solution_value + overcost;
            input.algorithm_formatter.update_bound(bound);
        } else {
            // Feasibility phase: 'relaxation_solution_value' isn't a valid
            // bound on the real problem here (every real column's
            // objective coefficient is zeroed in this phase's LP) -- the
            // only sound claim is a rigorous infeasibility proof from
            // 'solver->objective() + overcost' (sign convention matches
            // the dummy column cost: +1 Minimize, -1 Maximize).
            Value phase1_bound = solver->objective() + overcost;
            bool proven_infeasible
                = (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                (phase1_bound > FFOT_TOL):
                (phase1_bound < -FFOT_TOL);
            if (proven_infeasible) {
                input.algorithm_formatter.update_bound(
                        (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        std::numeric_limits<Value>::infinity():
                        -std::numeric_limits<Value>::infinity());
            }
        }
    };

    // Add newly found columns to the master LP -- shared by the regular
    // per-iteration pricing call below and the dual-oriented escalation
    // further down.
    auto add_columns_to_solver = [&input, &append_cut_coefficients, &solver_columns,
         &solver_generated_columns, &solver](
            const std::vector<std::shared_ptr<const Column>>& columns)
    {
        for (const std::shared_ptr<const Column>& column: columns) {
            std::vector<RowIdx> ri;
            std::vector<Value> rc;
            for (RowIdx row_pos = 0;
                    row_pos < (RowIdx)column->elements.size();
                    ++row_pos) {
                RowIdx i = column->elements[row_pos].row;
                Value c = column->elements[row_pos].coefficient;
                if (input.new_row_indices[i] < 0) {
                    throw std::logic_error("");
                }
                ri.push_back(input.new_row_indices[i]);
                rc.push_back(c);
            }
            append_cut_coefficients(*column, ri, rc);
            solver_columns.push_back(column);
            solver_generated_columns.insert(column);
            solver->add_column(
                    ri,
                    rc,
                    input.solve_feasibility? 0: column->objective_coefficient,
                    0,
                    std::numeric_limits<double>::infinity());
            input.output.number_of_columns_in_linear_subproblem++;
        }
    };

    // Filter a 'PricingType::Dual' pricing call's returned columns down to
    // genuinely new, correct-sign-reduced-cost ones -- the same filter the
    // regular per-iteration 'PricingType::Primal' call applies to its own
    // results, trimmed to a single pass since a 'Dual' call is never
    // retried under directional/Wentges smoothing the way 'Primal' is.
    auto filter_new_columns = [&input, &duals_out, &cut_duals](
            const std::vector<std::shared_ptr<const Column>>& all_columns)
    {
        std::vector<std::shared_ptr<const Column>> new_columns;
        for (const std::shared_ptr<const Column>& column: all_columns) {
            if (input.column_pool.find(column) != input.column_pool.end())
                continue;
            input.column_pool.insert(column);
            input.output.columns.push_back(column);
            Value rc = input.model.compute_reduced_cost(input.solve_feasibility, *column, duals_out, cut_duals);
            if (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                    && rc < -input.parameters.optimality_tolerance)
                new_columns.push_back(column);
            if (input.model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                    && rc > input.parameters.optimality_tolerance)
                new_columns.push_back(column);
        }
        return new_columns;
    };

    // 'true' iff the relaxation this attempt has converged to (so far)
    // still needs dummy columns to be feasible -- hoisted above the outer
    // loop below (rather than declared fresh each pass) so its last value
    // survives to 'result.has_dummy_column' once that loop is done.
    bool has_dummy_column = false;

    // Outer loop: run the (cheap, 'PricingType::Primal') inner column
    // generation loop below to convergence, then -- only when doing so
    // could change whether the current node can be cut, the pricing
    // solver actually has a 'PricingType::Dual' implementation worth
    // calling (see 'PricingSolver::has_pricing_bound'), and the whole
    // call's budget of (at most two) such calls isn't already spent --
    // try once to actually prove it. If that finds a new column, fold it
    // in and go around again (still 'Primal' from there on this trip,
    // since a 'Dual' call already consumed by this loop is gone for
    // good); if it doesn't, the bound/infeasibility proof from that call
    // (already applied via 'apply_overcost' below) stands, and there is
    // nothing left to try.
    for (;;) {

    for (Counter number_of_column_generation_iterations = 1;
            ;
            ++number_of_column_generation_iterations) {
        //std::cout << "number_of_column_generation_iterations " << number_of_column_generation_iterations << std::endl;

        // Solve LP
        auto start_lpsolve = std::chrono::high_resolution_clock::now();
        solver->solve();
        auto end_lpsolve = std::chrono::high_resolution_clock::now();
        auto time_span_lpsolve = std::chrono::duration_cast<std::chrono::duration<double>>(end_lpsolve - start_lpsolve);
        input.output.time_lpsolve += time_span_lpsolve.count();
        input.output.relaxation_solution_value = input.c0 + solver->objective();

        // The bound and the per-iteration display are computed after
        // pricing below, once 'overcost' reflects a reduced cost
        // computed at the same duals as this 'relaxation_solution_value'
        // (rather than the previous iteration's duals) — see there.
        input.output.number_of_column_generation_iterations++;

        // Check time.
        if (input.parameters.timer.needs_to_end())
            break;
        // Check iteration limit.
        if (input.parameters.maximum_number_of_iterations != -1
                && input.output.number_of_column_generation_iterations
                >= input.parameters.maximum_number_of_iterations) {
            break;
        }

        // Zero-pricing-calls short circuit: if this is Phase 1's very
        // first LP solve (built only from already-known columns) and it's
        // already dummy-free (verified: see 'verify_dummy_free' above),
        // stop right here rather than calling pricing at all -- further
        // column search is pointless once feasibility is already
        // achieved. Cheap to check since Phase 1 reruns every
        // cutting-plane round, where the previous round's columns almost
        // always still suffice. If verification finds it's not actually
        // dummy-free after all, fall through to the regular pricing call
        // below instead of breaking -- cheaper than bailing out to the
        // caller for a whole new attempt.
        if (input.solve_feasibility && number_of_column_generation_iterations == 1) {
            bool has_dummy_column_now = false;
            std::vector<std::shared_ptr<const Column>> nonzero_real_columns_now;
            for (ColIdx column_id = 0;
                    column_id < (ColIdx)solver_columns.size();
                    ++column_id) {
                if (std::abs(solver->primal(column_id)) < FFOT_TOL)
                    continue;
                if (solver_columns[column_id] == nullptr) {
                    has_dummy_column_now = true;
                    break;
                }
                nonzero_real_columns_now.push_back(solver_columns[column_id]);
            }
            if (!has_dummy_column_now && verify_dummy_free(nonzero_real_columns_now))
                break;
        }

        // Get duals from linear programming solver.
        for (RowIdx row_pos = 0; row_pos < input.new_number_of_rows; ++row_pos) {
            duals_out[input.new_rows[row_pos]] = solver->dual(row_pos);
        }
        for (CutIdx cut_pos = 0; cut_pos < (CutIdx)input.solver_cuts.size(); ++cut_pos) {
            cut_duals[cut_pos].second = solver->dual(input.new_number_of_rows + cut_pos);
        }

        if (!input.solve_feasibility
                && input.parameters.rounding_heuristic != Activation::Never
                && pricing_called_previous_iteration) {
            run_rounding_heuristic(rounding_heuristic_input);
        }

        std::vector<std::shared_ptr<const Column>> new_columns;
        std::vector<Value> pricing_lagrangian_column_values;

        // Search for new columns from the column pool.
        auto start_column_pool_search = std::chrono::high_resolution_clock::now();
        for (const std::shared_ptr<const Column>& column: input.column_pool) {

            // Don't add a column which is already in the LP.
            if (solver_generated_columns.find(column) != solver_generated_columns.end())
                continue;

            // Don't add a tabu column.
            if (input.parameters.tabu.find(column) != input.parameters.tabu.end())
                continue;

            // Add the column if its reduced cost is negative.
            Value rc = input.model.compute_reduced_cost(input.solve_feasibility, *column, duals_out, cut_duals);
            if (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                    && rc < -input.parameters.optimality_tolerance) {
                new_columns.push_back(column);
            }
            if (input.model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                    && rc > input.parameters.optimality_tolerance) {
                new_columns.push_back(column);
            }

        }
        auto end_column_pool_search = std::chrono::high_resolution_clock::now();
        input.output.time_column_pool_search += std::chrono::duration_cast<std::chrono::duration<double>>(
                end_column_pool_search - start_column_pool_search).count();

        // Cutting planes only ever shrink the relaxation's feasible region,
        // so this attempt's optimality-phase value can never end up worse
        // than the previous cutting-plane iteration's converged value (see
        // 'ColumnGenerationAttemptInput::
        // previous_cutting_plane_relaxation_value'). Once this iteration's
        // LP solve already matches or beats it -- even without a fresh
        // pricing call -- there is nothing to gain from calling the
        // (potentially expensive) pricing solver this round: the next
        // cutting-plane round's tighter cut set will refine things further
        // regardless.
        bool already_matches_previous_cutting_plane_round
            = !input.solve_feasibility
            && ((input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                    input.output.relaxation_solution_value >= input.previous_cutting_plane_relaxation_value:
                    input.output.relaxation_solution_value <= input.previous_cutting_plane_relaxation_value);

        bool call_pricing = new_columns.empty() && !already_matches_previous_cutting_plane_round;

        // Record, for the *next* iteration's rounding heuristic gate above,
        // whether real pricing is about to be called this iteration.
        pricing_called_previous_iteration = call_pricing;

        if (call_pricing) {
            // Search for new columns by solving the pricing problem.

            duals_in = duals_sep; // The last shall be the first.
            //std::cout << "alpha " << alpha << std::endl;
            for (Counter k = 1; ; ++k) {
                // Mispricing number.

                // Update global mispricing number.
                if (k > 1)
                    input.output.number_of_mispricings++;

                // Compute separation point.
                double alpha_cur = std::max(0.0, 1 - k * (1 - alpha) - FFOT_TOL);
                double beta = input.parameters.static_directional_smoothing_parameter;
                //std::cout << "alpha_cur " << alpha_cur << std::endl;
                if (number_of_column_generation_iterations == 1
                        || norm(input.new_rows, subgradient) == 0
                        // Shouldn't happen, but happens with Cplex.
                        || norm(input.new_rows, duals_in, duals_out) == 0
                        || k > 1
                        // No directional smoothing.
                        || (!input.parameters.automatic_directional_smoothing && beta == 0)) {

                    //std::cout << "compute duals_sep..." << std::endl;
                    for (RowIdx row_id: input.new_rows) {
                        //std::cout << " row " << row_id
                        //    << " dual_in " << duals_in[row_id]
                        //    << " dual_out " << duals_out[row_id]
                        //    << " alpha " << alpha_cur
                        //    << " dual_sep " << duals_sep[row_id]
                        //    << std::endl;
                        duals_sep[row_id]
                            = alpha_cur * duals_in[row_id]
                            + (1 - alpha_cur) * duals_out[row_id];
                    }

                } else {
                    // Directional smoothing.

                    // Compute π_tilde.
                    for (RowIdx row_id: input.new_rows) {
                        duals_tilde[row_id]
                            = alpha_cur * duals_in[row_id]
                            + (1 - alpha_cur) * duals_out[row_id];
                    }

                    // Compute π_g.
                    //std::cout << "compute duals_g..." << std::endl;
                    Value coef_g
                        = norm(input.new_rows, duals_in, duals_out)
                        / norm(input.new_rows, subgradient);
                    for (RowIdx row_id: input.new_rows) {
                        duals_g[row_id]
                            = duals_in[row_id]
                            + coef_g * subgradient[row_id];
                        //std::cout << " row " << row_id
                        //    << " dual_in " << duals_in[row_id]
                        //    << " subgradient " << subgradient[row_id]
                        //    << " coef_g " << coef_g
                        //    << " dual_g " << duals_g[row_id]
                        //    << std::endl;
                    }

                    // Compute β.
                    if (input.parameters.automatic_directional_smoothing) {
                        Value dot_product = 0;
                        for (RowIdx row_id: input.new_rows) {
                            dot_product
                                += (duals_out[row_id] - duals_in[row_id])
                                * (duals_g[row_id] - duals_in[row_id]);
                        }
                        beta = dot_product
                            / norm(input.new_rows, duals_in, duals_out)
                            / norm(input.new_rows, duals_in, duals_g);
                        //std::cout << "beta " << beta << std::endl;
                        //assert(beta >= 0);
                        beta = std::max(0.0, std::min(1.0, beta));
                    }

                    // Compute ρ.
                    //std::cout << "compute rho..." << std::endl;
                    for (RowIdx row_id: input.new_rows) {
                        rho[row_id]
                            = beta * duals_g[row_id]
                            + (1 - beta) * duals_out[row_id];
                        //std::cout << " row " << row_id
                        //    << " beta " << beta
                        //    << " dual_g " << duals_g[row_id]
                        //    << " dual_out " << duals_out[row_id]
                        //    << " rho " << rho[row_id]
                        //    << std::endl;
                    }

                    // Compute π_sep.
                    //std::cout << "compute duals_sep..." << std::endl;
                    //std::cout << "norm(input.new_rows, duals_in, duals_tilde) " << norm(input.new_rows, duals_in, duals_tilde) << std::endl;
                    //std::cout << "norm(input.new_rows, duals_in, rho) " << norm(input.new_rows, duals_in, rho) << std::endl;
                    Value norm_rho = norm(input.new_rows, duals_in, rho);
                    if (norm_rho < FFOT_TOL) {
                        // ρ ≈ π_in: directional adjustment is undefined;
                        // fall back to plain Wentges smoothing.
                        for (RowIdx row_id: input.new_rows)
                            duals_sep[row_id] = duals_tilde[row_id];
                    } else {
                        Value coef_sep
                            = norm(input.new_rows, duals_in, duals_tilde)
                            / norm_rho;
                        for (RowIdx row_id: input.new_rows) {
                            //std::cout << " row " << row_id
                            //    << " dual_in " << duals_in[row_id]
                            //    << " coef_sep " << coef_sep
                            //    << " rho " << rho[row_id]
                            //    << " dual_sep " << duals_sep[row_id]
                            //    << std::endl;
                            duals_sep[row_id]
                                = duals_in[row_id]
                                + coef_sep * (rho[row_id] - duals_in[row_id]);
                        }
                    }
                }

                // Call pricing solver on the computed separation point.
                auto start_pricing = std::chrono::high_resolution_clock::now();

                std::vector<std::shared_ptr<const Column>> all_columns;
                // Internal diving calls the pricing solver repeatedly
                // (potentially many times) to greedily fix columns as it
                // goes. Fall back to a single plain pricing call per
                // iteration whenever it's disabled -- always at 'Never',
                // and, at 'Initial', once this attempt is no longer the
                // very first cutting-plane round (see
                // 'ColumnGenerationParameters::internal_diving'): worth its
                // cost to seed things well before any cut exists, but not
                // worth repeating fresh every round after that.
                bool use_internal_diving =
                    (input.parameters.internal_diving == Activation::Always)
                    || (input.parameters.internal_diving == Activation::Initial
                            && input.cutting_plane_iteration == 0);
                if (!use_internal_diving) {
                    auto pricing_output = input.model.pricing_solver->solve_pricing(
                            input.solve_feasibility,
                            input.parameters.fixed_columns,
                            input.parameters.branching_decisions,
                            input.parameters.tabu,
                            duals_sep,
                            cut_duals,
                            PricingSolver::PricingType::Primal);
                    all_columns = pricing_output.columns;
                    overcost = pricing_output.overcost;
                    pricing_lagrangian_column_values = std::move(pricing_output.lagrangian_column_values);
                    for (const auto& column: all_columns)
                        input.model.check_generated_column(column);
                } else {
                    std::vector<Value> row_values_tmp = input.row_values;
                    std::vector<std::pair<std::shared_ptr<const Column>, Value>> fixed_columns_tmp = input.parameters.fixed_columns;
                    for (int i = 0;; ++i) {
                        auto pricing_output = input.model.pricing_solver->solve_pricing(
                                input.solve_feasibility,
                                fixed_columns_tmp,
                                input.parameters.branching_decisions,
                                input.parameters.tabu,
                                duals_sep,
                                cut_duals,
                                PricingSolver::PricingType::Primal);
                        std::vector<std::shared_ptr<const Column>> all_columns_tmp_0
                            = pricing_output.columns;
                        if (i == 0) {
                            overcost = pricing_output.overcost;
                            pricing_lagrangian_column_values = std::move(pricing_output.lagrangian_column_values);
                        }
                        for (const auto& column: all_columns_tmp_0)
                            input.model.check_generated_column(column);
                        std::vector<std::shared_ptr<const Column>> all_columns_tmp_1;
                        for (const auto& column: all_columns_tmp_0) {
                            if (column->elements.empty())
                                continue;
                            // Collecting a tabu column into 'all_columns'
                            // is harmless: from there it only ever reaches
                            // the master LP through the regular pricing
                            // loop below, which is safe regardless (its
                            // pool-membership check skips it -- a tabu'd
                            // column must already be in the pool).
                            all_columns.push_back(column);
                            // 'all_columns_tmp_1' is different: it feeds
                            // this diving loop's own greedy fixing below
                            // ('fixed_columns_tmp.push_back(...)'), which
                            // has no other check keeping a
                            // branching-excluded column out of what it
                            // fixes, and passing 'tabu' to 'solve_pricing'
                            // only lets the pricing solver *choose* to
                            // avoid these columns -- it isn't required to.
                            if (input.parameters.tabu.find(column) != input.parameters.tabu.end())
                                continue;
                            all_columns_tmp_1.push_back(column);
                        }
                        if (all_columns_tmp_1.empty())
                            break;

                        // Sort new columns by reduced cost.
                        std::sort(
                                all_columns_tmp_1.begin(),
                                all_columns_tmp_1.end(),
                                [&input, &duals_out, &cut_duals](
                                    const std::shared_ptr<const Column>& column_1,
                                    const std::shared_ptr<const Column>& column_2)
                                {
                                    Value rc1 = input.model.compute_reduced_cost(input.solve_feasibility, *column_1, duals_out, cut_duals);
                                    Value rc2 = input.model.compute_reduced_cost(input.solve_feasibility, *column_2, duals_out, cut_duals);
                                    if (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize) {
                                        return rc1 < rc2;
                                    } else {
                                        return rc1 > rc2;
                                    }
                                });
                        // Loop through new column by order of reduced costs.
                        bool has_fixed = false;
                        for (const auto& column: all_columns_tmp_1) {
                            // Compute the maximum number of copies of the
                            // column that can be added.
                            Value value = std::numeric_limits<Value>::infinity();
                            for (const LinearTerm& element: column->elements) {
                                if (element.coefficient > 0) {
                                    Value v
                                        = (input.model.rows[element.row].upper_bound
                                                - row_values_tmp[element.row])
                                        / element.coefficient;
                                    value = (std::min)(value, std::floor(v));
                                } else {
                                    Value v
                                        = (row_values_tmp[element.row]
                                                - input.model.rows[element.row].lower_bound)
                                        / (-element.coefficient);
                                    value = (std::min)(value, std::floor(v));
                                }
                            }
                            //std::cout << "value " << value << std::endl;

                            if (value > 0) {
                                // Update row values.
                                for (const LinearTerm& element: column->elements)
                                    row_values_tmp[element.row] += value * element.coefficient;
                                // Update fixed columns.
                                fixed_columns_tmp.push_back({column, value});
                                has_fixed = true;
                            }
                        }
                        if (!has_fixed)
                            break;
                    }
                }

                auto end_pricing = std::chrono::high_resolution_clock::now();
                auto time_span_pricing = std::chrono::duration_cast<std::chrono::duration<double>>(end_pricing - start_pricing);
                input.output.time_pricing += time_span_pricing.count();
                input.output.number_of_pricings++;
                if (alpha_cur == 0 && beta == 0)
                    input.output.number_of_no_stab_pricings++;

                // Look for negative reduced cost columns.
                for (const std::shared_ptr<const Column>& column: all_columns) {

                    // Discard columns which have already been generated.
                    // If they were worth adding to the LP, then they would
                    // have been added at the previous step (looking for
                    // column from the pool).
                    if (input.column_pool.find(column) != input.column_pool.end())
                        continue;

                    // Store these new columns.
                    input.column_pool.insert(column);
                  input.output.columns.push_back(column);

                  // Only add the ones with negative reduced cost.
                  Value rc = input.model.compute_reduced_cost(input.solve_feasibility, *column, duals_out, cut_duals);
                  // std::cout << "rc " << rc << std::endl;
                  if (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                          && rc < -input.parameters.optimality_tolerance)
                    new_columns.push_back(column);
                  if (input.model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                          && rc > input.parameters.optimality_tolerance)
                    new_columns.push_back(column);
                }

                if (!new_columns.empty() || (alpha_cur == 0.0 && beta == 0.0)) {
                    if (k == 1)
                        input.output.number_of_first_try_pricings++;
                    break;
                }

            }

        }

        // Update bound (or prove infeasibility), now that 'overcost'
        // reflects a reduced cost computed at the same duals ('duals_out')
        // as 'input.output.relaxation_solution_value' above -- giving the
        // tightest bound achievable from this iteration's master solve,
        // rather than the previous iteration's (still valid, since
        // 'relaxation_solution_value' only improves across iterations, but
        // needlessly loose). Checked every iteration (not just at
        // convergence) so the attempt can stop as soon as infeasibility is
        // provable, rather than continuing to iterate uselessly.
        apply_overcost(overcost);
        input.algorithm_formatter.print_column_generation_iteration(
                input.output.number_of_column_generation_iterations,
                input.output.number_of_columns_in_linear_subproblem,
                input.output.relaxation_solution_value,
                input.output.bound);
        input.parameters.iteration_callback(input.output);

        // Stop as soon as nothing further this attempt could find would
        // change the outcome (see 'Output::optimal()') -- a general
        // criterion, not specific to either phase: covers both "already
        // provably optimal" (a feasible incumbent matches the dual
        // bound) and "already provably infeasible" (the feasibility
        // phase branch above just proved it, reflected in 'bound' alone
        // since there's no incumbent to compare it to). Also stop once
        // 'bound' alone (from this or an earlier phase/round within the
        // same call) already reaches 'ColumnGenerationParameters::
        // objective_cutoff' -- the caller said it isn't interested in
        // anything this loose, regardless of whether a feasible incumbent
        // exists to match it against.
        if (input.output.optimal()
                || cutoff_reached(input.model, input.parameters, input.output.bound))
            break;

        // Stop the column generation procedure if no negative reduced cost
        // column has been found.
        //std::cout << "new_columns.size() " << new_columns.size() << std::endl;
        if (new_columns.empty())
            break;

        // Get Lagrangian constraint values Σ_k A·z*_k for the subgradient.
        // Use the pricer-provided values when available — necessary for
        // identical subproblems (e.g. bin packing with N bins) where the
        // pricer sets lagrangian_column_values[row] = N * A[row, z*],
        // analogous to returning overcost = N * rc*. Otherwise fall back
        // to summing the returned columns, which is correct when each
        // independent subproblem contributes exactly one column.
        std::fill(
                lagrangian_constraint_values.begin(),
                lagrangian_constraint_values.end(),
                0);
        if (!pricing_lagrangian_column_values.empty()) {
            lagrangian_constraint_values = pricing_lagrangian_column_values;
        } else {
            for (const std::shared_ptr<const Column>& column: new_columns)
                for (const LinearTerm& element: column->elements)
                    lagrangian_constraint_values[element.row] += element.coefficient;
        }

        // Compute subgradient at separation point.
        //std::cout << "update subgradient..." << std::endl;
        for (RowIdx row_id = 0; row_id < input.new_number_of_rows; ++row_id) {
            subgradient[input.new_rows[row_id]]
                = std::min(
                        0.0,
                        input.new_row_upper_bounds[row_id]
                        - lagrangian_constraint_values[input.new_rows[row_id]])
                + std::max(
                        0.0,
                        input.new_row_lower_bounds[row_id]
                        - lagrangian_constraint_values[input.new_rows[row_id]]);
            //std::cout << " row " << row_id
            //    << " lb " << input.new_row_lower_bounds[row_id]
            //    << " ub " << input.new_row_upper_bounds[row_id]
            //    << " val " << lagrangian_constraint_values[input.new_rows[row_id]]
            //    << std::endl;
        }

        // Adjust alpha.
        if (input.parameters.self_adjusting_wentges_smoothing
                && norm(input.new_rows, duals_in, duals_sep) != 0) {
            //for (RowIdx i: input.new_rows)
            //    std::cout
            //        << "i " << i
            //        << " y " << lagrangian_constraint_values[i]
            //        << " dual_in " << duals_in[i]
            //        << " dual_out " << duals_out[i]
            //        << " dual_sep " << duals_sep[i]
            //        << " diff " << duals_sep[i] - duals_in[i]
            //        << " l " << input.new_row_lower_bounds[i]
            //        << " u " << input.new_row_upper_bounds[i]
            //        << " g " << subgradient[i]
            //        << std::endl;

            // Compute g^sep · (π^out - π^in) per Pessoa et al. (2018),
            // Section 4. A positive dot product means the subgradient at
            // the sep-point is aligned with the direction toward π^out, so
            // a larger step would improve the dual bound: decrease α (less
            // smoothing). Note: Table 1 of the paper has f_incr/f_decr
            // swapped in Step 4; the body text on p. 347 is correct.
            Value v = 0;
            for (RowIdx row_id: input.new_rows)
                v += subgradient[row_id] * (duals_out[row_id] - duals_in[row_id]);
            //std::cout << "v " << v << std::endl;

            // Update alpha.
            if (v > 0) {
                alpha = std::max(0.0, alpha - 0.1);
            } else {
                alpha = std::min(0.99, alpha + (1.0 - alpha) * 0.1);
            }
        }

        // Add new columns to the linear program.
        add_columns_to_solver(new_columns);
    }

    // Compute relaxation solution.
    SolutionBuilder solution_builder;
    solution_builder.set_model(input.model);
    for (const auto& p: input.parameters.fixed_columns) {
        solution_builder.add_column(
                p.first,
                p.second);
    }
    has_dummy_column = false;
    // Real columns with a non-null value in this solution, kept only to
    // feed the dummy-free verification below -- not needed once that's
    // done.
    std::vector<std::shared_ptr<const Column>> nonzero_real_columns;
    for (ColIdx column_id = 0;
            column_id < (ColIdx)solver_columns.size();
            ++column_id) {
        if (std::abs(solver->primal(column_id)) < FFOT_TOL)
            continue;
        if (solver_columns[column_id] == nullptr) {
            has_dummy_column = true;
            //RowIdx row_orig_id = input.new_rows[dummy_column_rows[column_id]];
            //std::cout << "dummy column id " << column_id
            //    << " row_lp " << dummy_column_rows[column_id]
            //    << " row_orig " << row_orig_id
            //    << " name " << input.model.rows[row_orig_id].name
            //    << " value " << solver->primal(column_id) << std::endl
            //    << std::endl;
        } else {
            if (solver->primal(column_id) > solver_columns[column_id]->upper_bound + FFOT_TOL) {
                std::stringstream ss;
                ss << "column_id " << column_id << std::endl;
                ss << "solver->primal(column_id) " << solver->primal(column_id) << std::endl;
                ss << "*solver_columns[column_id] " << *solver_columns[column_id] << std::endl;
                ss << "solver_columns[column_id]->upper_bound " << solver_columns[column_id]->upper_bound << std::endl;
                throw std::runtime_error(ss.str());
            }
            solution_builder.add_column(
                    solver_columns[column_id],
                    solver->primal(column_id));
            if (input.solve_feasibility)
                nonzero_real_columns.push_back(solver_columns[column_id]);
        }
    }

    // The magnitude check above can under-report dummy-column usage (see
    // 'verify_dummy_free' above): confirm the dummy-free verdict before
    // trusting it, so a wrong one is reported back as 'has_dummy_column'
    // still true (the dual-oriented escalation right below gets a fair
    // shot at it, then the caller retries with cutting planes) instead of
    // falling through to a Phase 2 doomed to the same infeasibility.
    if (input.solve_feasibility && !has_dummy_column && !verify_dummy_free(nonzero_real_columns))
        has_dummy_column = true;

    // Check time.
    if (input.parameters.timer.needs_to_end()) {
        input.output.relaxation_solution = solution_builder.build();
        result.stop_now = true;
        return result;
    }
    // Check iteration limit.
    if (input.parameters.maximum_number_of_iterations != -1
            && input.output.number_of_column_generation_iterations
            > input.parameters.maximum_number_of_iterations) {
        input.output.relaxation_solution = solution_builder.build();
        result.stop_now = true;
        return result;
    }

    // Every attempt that reaches this point (as opposed to the 'stop_now'
    // returns above) has a relaxation solution the caller needs to see,
    // so record it unconditionally -- including Phase 1's own result when
    // it succeeds, even though it's immediately superseded by Phase 2's
    // call right after; harmless, and simpler than only conditionally
    // assigning it. 'relaxation_solution_is_feasible' is '!has_dummy_column'
    // uniformly in both phases: Phase 2 never has dummy columns in its LP
    // at all, so 'has_dummy_column' is always 'false' there by
    // construction.
    input.output.relaxation_solution = solution_builder.build();
    input.output.relaxation_solution_is_feasible = !has_dummy_column;

    // 'PricingType::Primal' pricing alone has now converged (the inner
    // loop above only breaks once it finds nothing further, or the cutoff
    // was already reached from a bound proven earlier). Whether that's
    // worth escalating to a 'PricingType::Dual' pricing call depends on
    // whether succeeding there could actually change the outcome for this
    // node:
    // - Feasibility phase: still needs dummy columns, i.e. looks
    //   infeasible -- exactly the case 'PricingType::Dual' exists to
    //   (dis)prove.
    // - Optimality phase: the relaxation value reached is already no
    //   better than 'ColumnGenerationParameters::objective_cutoff' -- i.e.
    //   even in the best case (this value turning out to already be the
    //   true LP optimum), the node would be cut, so it's worth spending
    //   the effort to find out for sure.
    // Only tried at all if the pricing solver actually has something to
    // offer there ('PricingSolver::has_pricing_bound'), and capped at
    // 'input.number_of_dual_pricing_calls < 2' (shared across the whole
    // 'column_generation()' call, not just this attempt).
    bool eligible_for_dual_pricing = (input.solve_feasibility)?
        has_dummy_column:
        cutoff_reached(input.model, input.parameters, input.output.relaxation_solution_value);
    if (eligible_for_dual_pricing
            && input.model.pricing_solver->has_pricing_bound()
            && input.number_of_dual_pricing_calls < 2) {
        ++input.number_of_dual_pricing_calls;

        // A single direct call, deliberately bypassing
        // 'parameters.internal_diving' (see its use further up, gating
        // 'PricingType::Primal' calls only): diving's repeated greedy
        // fix/price/fix calls are a cheap-primal-pricing technique, not
        // something to run under an expensive 'PricingType::Dual' solve.
        auto start_pricing = std::chrono::high_resolution_clock::now();
        auto pricing_output = input.model.pricing_solver->solve_pricing(
                input.solve_feasibility,
                input.parameters.fixed_columns,
                input.parameters.branching_decisions,
                input.parameters.tabu,
                duals_out,
                cut_duals,
                PricingSolver::PricingType::Dual);
        auto end_pricing = std::chrono::high_resolution_clock::now();
        double dual_pricing_time = std::chrono::duration_cast<std::chrono::duration<double>>(
                end_pricing - start_pricing).count();
        input.output.time_pricing += dual_pricing_time;
        input.output.time_dual_pricing += dual_pricing_time;
        input.output.number_of_pricings++;
        input.output.number_of_dual_pricings++;
        for (const auto& column: pricing_output.columns)
            input.model.check_generated_column(column);

        // Feeds 'input.output.bound' the same way a regular pricing call
        // does -- this is what actually proves the cut (infeasibility, or
        // the bound reaching the cutoff) when this 'PricingType::Dual'
        // call is exact and finds nothing new below.
        apply_overcost(pricing_output.overcost);

        std::vector<std::shared_ptr<const Column>> dual_new_columns
            = filter_new_columns(pricing_output.columns);
        if (!dual_new_columns.empty()) {
            add_columns_to_solver(dual_new_columns);
            continue;
        }
    }
    break;
    }

    // Only meaningful to check for Phase 2: guaranteed feasible by
    // construction (seeded from Phase 1's own optimal basis, no dummy
    // columns in this phase's LP at all) -- Phase 1 is explicitly allowed
    // to still need dummy columns, that's exactly what 'has_dummy_column'
    // tracks.
    if (!input.solve_feasibility && !input.output.relaxation_solution.feasible_relaxation()) {
        throw std::logic_error(
                "columngenerationsolver::column_generation: "
                "infeasible relaxation solution.");
    }

    result.has_dummy_column = has_dummy_column;
    return result;
}

}

const ColumnGenerationOutput columngenerationsolver::column_generation(
        const Model& model,
        const ColumnGenerationParameters& parameters)
{
    // Initial display.
    ColumnGenerationOutput output(model);
    AlgorithmFormatter algorithm_formatter(
            model,
            parameters,
            output);
    algorithm_formatter.start("Column generation");
    algorithm_formatter.print_column_generation_header();

    RowIdx number_of_rows = model.rows.size();
    //std::cout << "m " << m << std::endl;
    //std::cout << "parameters.fixed_columns.size() " << parameters.fixed_columns.size() << std::endl;

    // Compute row values.
    //std::cout << "Compute row values..." << std::endl;
    std::vector<Value> row_values(number_of_rows, 0.0);
    Value c0 = 0.0;
    for (auto p: parameters.fixed_columns) {
        //std::cout << *p.first << std::endl;
        //std::cout << p.second << std::endl;
        const Column& column = *(p.first);
        Value value = p.second;
        for (const LinearTerm& element: column.elements)
            row_values[element.row] += value * element.coefficient;
        c0 += value * column.objective_coefficient;
    }

    // Compute fixed rows.
    //std::cout << "Compute fixed rows..." << std::endl;
    std::vector<RowIdx> new_row_indices(number_of_rows, -2);
    std::vector<RowIdx> new_rows;
    RowIdx row_pos = 0;
    for (RowIdx row_id = 0; row_id < number_of_rows; ++row_id) {
        //std::cout
        //    << "row " << row
        //    << " lb " << model.row_lower_bounds[row]
        //    << " val " << row_values[row]
        //    << " ub " << model.row_upper_bounds[row]
        //    << std::endl;
        if (model.rows[row_id].coefficient_lower_bound >= 0
                && row_values[row_id] > model.rows[row_id].upper_bound + model.rows[row_id].feasibility_tolerance) {
            // Infeasible: the fixed columns alone already violate this row.
            algorithm_formatter.update_bound(
                    (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        std::numeric_limits<Value>::infinity():
                        -std::numeric_limits<Value>::infinity());
            return output;
        }
        if (model.rows[row_id].coefficient_lower_bound >= 0
                && row_values[row_id] >= model.rows[row_id].upper_bound - model.rows[row_id].feasibility_tolerance) {
            //std::cout
            //    << "row " << row_id
            //    << " ub " << model.rows[row_id].upper_bound
            //    << " val " << row_values[row_id]
            //    << std::endl;
            continue;
        }
        new_row_indices[row_id] = row_pos;
        new_rows.push_back(row_id);
        row_pos++;
    }
    RowIdx new_number_of_rows = row_pos;
    //std::cout << "new_number_of_rows: " << new_number_of_rows << std::endl;
    if (new_number_of_rows == 0)
        return output;

    // Compute new row bounds.
    //std::cout << "Compute new row bounds..." << std::endl;
    std::vector<Value> new_row_lower_bounds(new_number_of_rows);
    std::vector<Value> new_row_upper_bounds(new_number_of_rows);
    for (RowIdx row_id = 0; row_id < new_number_of_rows; ++row_id) {
        new_row_lower_bounds[row_id]
            = model.rows[new_rows[row_id]].lower_bound
            - row_values[new_rows[row_id]];
        new_row_upper_bounds[row_id]
            = model.rows[new_rows[row_id]].upper_bound
            - row_values[new_rows[row_id]];
        //std::cout << "row " << row << " lb " << new_row_lower_bounds[row] << " ub " << new_row_upper_bounds[row] << std::endl;
    }

    // We never keep a generated column identical to a previously generated
    // column.
    // This set is used to store all the generated column.
    ColumnHasher column_hasher(model);
    std::unordered_set<std::shared_ptr<const Column>,
                       const ColumnHasher&,
                       const ColumnHasher&> column_pool(0, column_hasher, column_hasher);
    // We first add to it the columns from the input column pool.
    for (const auto& column: parameters.column_pool) {

        bool ok = true;
        for (const LinearTerm& element: column->elements) {
            // The column might not be feasible.
            // For example, it corresponds to the same bin / machine that a
            // currently fixed column or it contains an item / job also
            // included in a currently fixed column.
            if (model.rows[element.row].coefficient_lower_bound >= 0
                    && column->type == VariableType::Integer
                    && row_values[element.row] + element.coefficient
                    > model.rows[element.row].upper_bound) {
                ok = false;
                break;
            }
            if (new_row_indices[element.row] < 0) {
                ok = false;
                break;
            }
        }
        if (!ok)
            continue;

        column_pool.insert(column);
    }

    // Active cuts, each paired with the relaxation's objective value at the
    // point it joined -- a cut from 'parameters.initial_cuts' (inherited
    // from a parent node, not "added" this call at all) gets a sense-aware
    // sentinel that's trivially already exceeded, matching how such a cut
    // could always be removed on first sight of slack before this scheme
    // existed. Grows (with a genuine baseline this time) as cutting-plane
    // rounds find violated cuts below, and is how the removal pass further
    // down decides a cut is stale enough to drop: see there.
    std::vector<std::pair<std::shared_ptr<const Cut>, Value>> solver_cuts;
    for (const std::shared_ptr<const Cut>& cut: parameters.initial_cuts) {
        solver_cuts.push_back({
                cut,
                (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                -std::numeric_limits<Value>::infinity():
                std::numeric_limits<Value>::infinity()});
    }

    // 'ColumnGenerationOutput::cuts' is a plain cut list (no caller needs
    // to see the internal per-cut removal bookkeeping) -- this is how
    // 'solver_cuts' gets projected down to it, at every point this
    // function can return.
    auto extract_cuts = [](const std::vector<std::pair<std::shared_ptr<const Cut>, Value>>& cuts)
    {
        std::vector<std::shared_ptr<const Cut>> result;
        result.reserve(cuts.size());
        for (const auto& p: cuts)
            result.push_back(p.first);
        return result;
    };

    // Cuts the most recent 'separate_cuts' call confirmed violated but
    // didn't activate this round (only the single most violated cut ever
    // gets activated per round -- see below). Checked first, before
    // calling 'separate_cuts' again, so a cut that isn't needed yet but
    // will be in a round or two doesn't cost a fresh (potentially
    // expensive) separation call. Replaced wholesale by the next
    // 'separate_cuts' call's own results rather than accumulated --
    // unlike a real cut pool, this deliberately doesn't remember a cut
    // beyond the batch it was found in: once none of a batch's cuts are
    // violated any more, a later round's fresh 'separate_cuts' call is
    // trusted to rediscover whatever's still relevant, rather than an
    // ever-growing history being kept across the whole call to avoid that.
    std::vector<std::shared_ptr<const Cut>> pending_separated_cuts;

    std::vector<std::shared_ptr<const Column>> initial_columns = parameters.initial_columns;

    // Budget of how many more times 'solve_pricing' may still be called
    // with 'PricingSolver::PricingType::Dual' this whole call (see
    // 'ColumnGenerationAttemptInput::number_of_dual_pricing_calls' and
    // 'run_column_generation_attempt'). Capped at 2 -- see
    // 'PricingSolver::solve_pricing'.
    Counter number_of_dual_pricing_calls = 0;

    // See 'ColumnGenerationAttemptInput::previous_cutting_plane_relaxation_
    // value': unreachable at the first iteration, since there is no
    // previous round yet.
    Value previous_cutting_plane_relaxation_value
        = (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
        std::numeric_limits<Value>::infinity():
        -std::numeric_limits<Value>::infinity();

    // Loop for cutting planes.
    // After the dummy-column loop below converges to a feasible relaxation
    // (no dummy column left), if cutting planes are enabled, cuts are
    // separated from that relaxation. If any are found, the whole master LP
    // is rebuilt from scratch (like a dummy-column retry) with the enlarged
    // cut set and re-optimized. The loop stops when no more violated cuts
    // are found, cutting planes are disabled, or the cutting-plane
    // iteration limit is reached.
    for (Counter cutting_plane_iteration = 0; ; ++cutting_plane_iteration) {

        // Round 0 doesn't need its own header -- it's already unambiguous
        // right after the top-level "Column generation" header, with no
        // prior round to distinguish it from.
        if (cutting_plane_iteration > 0) {
            algorithm_formatter.print_column_generation_cutting_plane_header(cutting_plane_iteration);
            parameters.cutting_plane_callback(cutting_plane_iteration);
        }

        // Compute residual cut bounds, after subtracting the contribution of
        // fixed columns (mirrors the row residual-bound computation above).
        std::vector<Value> new_cut_lower_bounds(solver_cuts.size());
        std::vector<Value> new_cut_upper_bounds(solver_cuts.size());
        for (CutIdx cut_pos = 0; cut_pos < (CutIdx)solver_cuts.size(); ++cut_pos) {
            Value cut_fixed_value = 0.0;
            for (const auto& p: parameters.fixed_columns)
                cut_fixed_value += p.second * model.pricing_solver->coefficient(*solver_cuts[cut_pos].first, *p.first);
            new_cut_lower_bounds[cut_pos] = solver_cuts[cut_pos].first->lower_bound - cut_fixed_value;
            new_cut_upper_bounds[cut_pos] = solver_cuts[cut_pos].first->upper_bound - cut_fixed_value;
        }

        // Appends the coefficients of 'column' in the active cuts to 'ri'/'rc',
        // at row indices following the model rows.
        auto append_cut_coefficients = [&model, &solver_cuts, new_number_of_rows](
                const Column& column,
                std::vector<RowIdx>& ri,
                std::vector<Value>& rc)
        {
            for (CutIdx cut_pos = 0; cut_pos < (CutIdx)solver_cuts.size(); ++cut_pos) {
                Value coef = model.pricing_solver->coefficient(*solver_cuts[cut_pos].first, column);
                if (coef != 0.0) {
                    ri.push_back(new_number_of_rows + cut_pos);
                    rc.push_back(coef);
                }
            }
        };

        // Two-phase method: Phase 1 (feasibility) searches for a dummy-column-
        // free relaxation using a fixed, non-escalating dummy weight and a
        // zeroed real objective; Phase 2 (optimality) re-solves the same
        // feasible region with the real objective restored and no dummy
        // columns at all. Phase 2 is only reached once Phase 1 has actually
        // converged dummy-free, so its relaxation is guaranteed feasible by
        // construction.
        for (bool solve_feasibility : {true, false}) {

            algorithm_formatter.print_column_generation_phase_header(solve_feasibility);
            parameters.phase_callback(solve_feasibility);

            ColumnGenerationAttemptInput attempt_input{
                    model,
                    parameters,
                    number_of_rows,
                    row_values,
                    c0,
                    new_row_indices,
                    new_rows,
                    new_number_of_rows,
                    new_row_lower_bounds,
                    new_row_upper_bounds,
                    solver_cuts,
                    new_cut_lower_bounds,
                    new_cut_upper_bounds,
                    initial_columns,
                    solve_feasibility,
                    cutting_plane_iteration,
                    number_of_dual_pricing_calls,
                    previous_cutting_plane_relaxation_value,
                    column_pool,
                    output,
                    algorithm_formatter};
            ColumnGenerationAttemptResult attempt_result
                = run_column_generation_attempt(attempt_input);

            if (attempt_result.stop_now) {
                output.cuts = extract_cuts(solver_cuts);
                algorithm_formatter.end();
                return output;
            }

            // Warm-start columns for whichever attempt runs next (the other
            // phase; another cutting-plane round; an escalated pricing level)
            // -- always this attempt's own real columns, regardless of which
            // of those it turns out to be: 'parameters.initial_columns' plus
            // every real (non-static/fixed, i.e. present in 'column_pool')
            // column 'output.relaxation_solution' used, already set above by
            // 'run_column_generation_attempt'.
            initial_columns = parameters.initial_columns;
            for (const auto& p: output.relaxation_solution.columns())
                if (column_pool.find(p.first) != column_pool.end())
                    initial_columns.push_back(p.first);

            if (solve_feasibility) {
                // If Phase 1 converged without needing any dummy column, fall
                // through to Phase 2 (already warm-started above from Phase
                // 1's own optimal basis).
                if (!attempt_result.has_dummy_column)
                    continue;

                // Otherwise, dummy columns persist even at Phase 1's fixed
                // weight: stop the phase loop here, Phase 2 needs Phase 1 to
                // have succeeded first. 'run_column_generation_attempt' has
                // already tested (and, if proven, reported through
                // 'algorithm_formatter') a rigorous infeasibility bound, and
                // recorded the outcome in 'output.relaxation_solution'/
                // 'output.relaxation_solution_is_feasible' either way -- an
                // inconclusive result (no bound, or the bound doesn't prove
                // infeasibility) leaves 'output.relaxation_solution_is_feasible'
                // as the only signal, same as running out of time.
                break;
            }
            // Phase 2: 'run_column_generation_attempt' already validated
            // (guaranteed feasible by construction, seeded from Phase 1's own
            // optimal basis, no dummy columns in this phase's LP at all) and
            // recorded the result in 'output.relaxation_solution'/
            // 'output.relaxation_solution_is_feasible' above -- nothing left
            // to do here.
        }

        // Record this round's converged optimality-phase value for the
        // next cutting-plane iteration's 'previous_cutting_plane_
        // relaxation_value' (see its doc comment) -- only meaningful once
        // Phase 2 actually ran and reached a genuinely feasible relaxation;
        // left untouched otherwise (Phase 1 alone failing leaves nothing
        // comparable to record).
        if (output.relaxation_solution_is_feasible)
            previous_cutting_plane_relaxation_value = output.relaxation_solution_value;

        // Already solved to optimality -- either Phase 1 just rigorously
        // proved infeasibility above ('output.optimal()' reads that exact
        // proof, since no feasible incumbent exists yet -- see its doc
        // comment), or a feasible incumbent (e.g. from the rounding heuristic,
        // which can run during Phase 2 above) already matches 'bound' -- or
        // 'bound' alone already reaches 'parameters.objective_cutoff', so
        // the caller isn't interested in anything looser than this
        // regardless of any incumbent. Either way, nothing further to try:
        // stop right away, don't bother separating cuts or trying the
        // dual-oriented pricing escalation.
        if (output.optimal()
                || cutoff_reached(model, parameters, output.bound))
            break;

        // Cutting planes disabled for this call, or the iteration limit already
        // reached: if the relaxation is genuinely feasible (Phase 2 succeeded),
        // stop here, exactly like before cuts existed. If it's still only
        // inconclusive (Phase 1 failed but wasn't proven infeasible), the
        // dual-oriented pricing escalation inside the next
        // 'run_column_generation_attempt' call is an independent knob,
        // unrelated to cutting planes, and still worth a try.
        bool try_cutting_planes = (parameters.cutting_planes != Activation::Never)
            && (parameters.maximum_number_of_cutting_plane_iterations == -1
                    || cutting_plane_iteration < parameters.maximum_number_of_cutting_plane_iterations);
        if (!try_cutting_planes && output.relaxation_solution_is_feasible)
            break;

        std::vector<std::shared_ptr<const Cut>> new_cuts;
        if (try_cutting_planes) {
            // Look for a still-violated cut among 'pending_separated_cuts'
            // (the last 'separate_cuts' call's own leftovers) before ever
            // calling the (potentially expensive) separation routine
            // again -- a cut that isn't needed yet but was violated a
            // round or two ago (see the removal pass below) is often
            // needed again very soon, at no cost beyond evaluating it
            // against the current relaxation. Only the single most
            // violated one gets activated, exactly like a freshly
            // separated cut below: adding every violated cut at once
            // would grow the master LP faster than the relaxation
            // actually needs, since a later round can always add another
            // once this one alone no longer suffices.
            {
                std::shared_ptr<const Cut> best_cut = nullptr;
                Value best_violation = 0.0;
                for (const std::shared_ptr<const Cut>& cut: pending_separated_cuts) {
                    bool already_active = std::any_of(
                            solver_cuts.begin(), solver_cuts.end(),
                            [&cut](const std::pair<std::shared_ptr<const Cut>, Value>& p)
                            {
                                return p.first == cut;
                            });
                    if (already_active)
                        continue;
                    if (!cut_is_violated(model, *cut, output.relaxation_solution))
                        continue;
                    Value violation = cut_violation(model, *cut, output.relaxation_solution);
                    if (best_cut == nullptr || violation > best_violation) {
                        best_cut = cut;
                        best_violation = violation;
                    }
                }
                if (best_cut != nullptr)
                    new_cuts.push_back(best_cut);
            }

            // Only once that has nothing to offer, separate cuts from the
            // current relaxation solution -- the full feasible one from
            // Phase 2, or, if Phase 1 stayed inconclusive instead, the
            // partial one it left behind (dummy columns excluded from it
            // by construction). A cut found on that partial solution
            // might let a later Phase 1 attempt restore feasibility where
            // this one couldn't.
            if (new_cuts.empty()) {
                auto start_separation = std::chrono::high_resolution_clock::now();
                std::vector<std::shared_ptr<const Cut>> separated_cuts
                    = model.pricing_solver->separate_cuts(output.relaxation_solution);
                auto end_separation = std::chrono::high_resolution_clock::now();
                auto time_span_separation = std::chrono::duration_cast<std::chrono::duration<double>>(end_separation - start_separation);
                output.time_separation += time_span_separation.count();

                // Never trust a 'PricingSolver''s own notion of
                // "violated" -- it's a pluggable interface, so every cut
                // this call returns is independently confirmed here
                // before being trusted at all. Replaces
                // 'pending_separated_cuts' wholesale (see its own doc
                // comment) with every cut that passes -- not just the one
                // chosen to stay active below -- so a round or two from
                // now can pick up one of this same batch's other cuts
                // without a fresh separation call.
                pending_separated_cuts.clear();
                std::shared_ptr<const Cut> best_cut = nullptr;
                Value best_violation = 0.0;
                for (const std::shared_ptr<const Cut>& cut: separated_cuts) {
                    if (!cut_is_violated(model, *cut, output.relaxation_solution))
                        continue;
                    pending_separated_cuts.push_back(cut);
                    Value violation = cut_violation(model, *cut, output.relaxation_solution);
                    if (best_cut == nullptr || violation > best_violation) {
                        best_cut = cut;
                        best_violation = violation;
                    }
                }
                if (best_cut != nullptr)
                    new_cuts.push_back(best_cut);
            }

            // Remove cuts that are no longer active: their value at the
            // current relaxation solution has slack on both sides, more than
            // their own 'feasibility_tolerance', relative to their bounds (the
            // same check 'Row::feasibility_tolerance' does for rows). Checking
            // the value rather than the dual avoids a false "inactive" reading
            // under LP degeneracy — common in exactly the set-partitioning-
            // style formulations this framework targets — where a constraint
            // can be geometrically at its bound yet still be reported with a
            // zero dual, because multiple dual solutions can correspond to the
            // same primal optimum. Non-robust cuts especially can make pricing
            // significantly harder, so don't keep paying for ones that aren't
            // helping. Applies just as well to an inconclusive (partial)
            // relaxation as to a genuinely feasible one: a cut's value against
            // the real columns used so far is well-defined and meaningful
            // regardless of whether every row is already fully satisfied by
            // them.
            //
            // On top of that, a cut is only actually dropped once the
            // relaxation has moved on by more than 1% (sense-aware, relative
            // to the magnitude of the value recorded when it joined
            // 'solver_cuts', with an absolute 'FFOT_TOL' floor so a
            // near-zero baseline doesn't collapse the threshold to
            // nothing) -- otherwise a cut that's merely momentarily slack,
            // with nothing else having genuinely changed since it was
            // added, would get removed only to need re-deriving again
            // almost immediately, for no real gain. This also naturally
            // prevents the remove/rebuild/re-add/rebuild/remove cycling a
            // purely slack-based check alone would risk, without having to
            // separately recognize a cut across being removed and later
            // re-added, possibly as a different 'Cut' instance for the
            // same constraint (see 'separate_cuts'). A cut inherited via
            // 'parameters.initial_cuts' (not "added" this call at all) has
            // no such baseline -- its sense-aware sentinel (see where
            // 'solver_cuts' is built) always counts as "moved on enough",
            // matching how a cut could always be removed on first sight of
            // slack before this scheme existed.
            //
            // That 1% gate only guards against cycling that could still
            // cost something later *this* call -- it's dropped entirely
            // once 'new_cuts' is empty here, since by construction that
            // only happens after a genuine 'separate_cuts' call (not just
            // the cheap 'pending_separated_cuts' check above) has just
            // confirmed nothing is violated at this exact relaxation: the
            // loop is ending this round regardless (see the break below),
            // so there is no future re-derivation cost left to protect a
            // slack cut's baseline against -- may as well hand back the
            // leanest possible 'output.cuts' to whoever called this. Never
            // worsens the relaxation value either way: dropping a cut with
            // genuine slack at the optimum can only loosen constraints the
            // solution wasn't using, not change what was achievable.
            //
            // 'output.cuts' (see 'ColumnGenerationOutput::cuts') mirrors
            // 'solver_cuts' by the time this call returns, so a cut dropped
            // here — whether it came in via 'parameters.initial_cuts' or was
            // newly separated this call — is dropped from 'output.cuts' too,
            // and a caller feeding 'output.cuts' into a follow-up call won't
            // keep reinstating it.
            bool minimize = (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize);
            Value current_value = output.relaxation_solution.objective_value();
            Value no_baseline_sentinel = (minimize)?
                -std::numeric_limits<Value>::infinity():
                std::numeric_limits<Value>::infinity();
            std::vector<std::pair<std::shared_ptr<const Cut>, Value>> still_solver_cuts;
            for (const auto& p: solver_cuts) {
                const std::shared_ptr<const Cut>& cut = p.first;
                Value value_at_addition = p.second;
                Value value = cut_value(model, *cut, output.relaxation_solution);

                bool has_slack_below = (value > cut->lower_bound + cut->feasibility_tolerance);
                bool has_slack_above = (value < cut->upper_bound - cut->feasibility_tolerance);
                bool eligible_for_removal = has_slack_below && has_slack_above;

                if (eligible_for_removal
                        && !new_cuts.empty()
                        && value_at_addition != no_baseline_sentinel) {
                    Value progress = (minimize)?
                        (current_value - value_at_addition):
                        (value_at_addition - current_value);
                    Value progress_threshold = 0.01 * std::abs(value_at_addition) + FFOT_TOL;
                    eligible_for_removal = (progress > progress_threshold);
                }

                if (eligible_for_removal) {
                    // Not tracked any further: since 'new_cuts.empty()'
                    // alone now decides whether the loop keeps going (see
                    // below), a removal happening here can never be the
                    // reason for another round.
                } else {
                    still_solver_cuts.push_back(p);
                }
            }
            solver_cuts = std::move(still_solver_cuts);
        }

        if (new_cuts.empty()) {
            // Nothing found this round (cutting planes disabled/capped, or
            // a genuinely empty attempt): nothing left to try -- including,
            // as of the removal pass above, any cut that was merely slack.
            // (This is also, incidentally, the last chance for the next
            // round's 'run_column_generation_attempt' call -- there won't
            // be one -- so the dual-oriented pricing escalation inside the
            // *current* round's attempts, gated by the same
            // 'number_of_dual_pricing_calls < 2' budget, already got
            // whatever shot at proving a cut this call is going to give
            // it.)
            break;
        } else {
            // Baseline each newly-activated cut against this round's own
            // converged relaxation value -- the value the system had right
            // as it's joining, before it can have had any effect yet (see
            // 'solver_cuts''s own doc comment for how the removal pass
            // further up uses this).
            Value new_cut_baseline_value = output.relaxation_solution.objective_value();
            for (const std::shared_ptr<const Cut>& cut: new_cuts)
                solver_cuts.push_back({cut, new_cut_baseline_value});
            output.number_of_cutting_plane_iterations++;
        }

        // Rebuild the master LP from scratch next round (with the enlarged cut
        // set, if any -- unchanged otherwise, if this round only escalated the
        // pricing level): 'initial_columns' is already warm-started from this
        // attempt's own relaxation solution, set right after it ran, above.
    }

    output.cuts = extract_cuts(solver_cuts);
    algorithm_formatter.end();
    return output;
}
