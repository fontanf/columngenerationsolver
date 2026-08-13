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
 * Input/state for the inline rounding heuristic (see
 * 'ColumnGenerationParameters::rounding_heuristic'): the pieces of
 * 'column_generation()'s state it needs to read and/or mutate, bundled
 * here since there are too many for a plain parameter list to stay
 * readable — plain data only, no behavior (see the free functions below).
 *
 * Built once (references stay valid: the referenced containers get
 * mutated in place across iterations, never reallocated to a new object),
 * then 'run_rounding_heuristic()' is called every iteration. Never
 * touches the master LP, 'active_cuts', or the real
 * 'parameters.fixed_columns' — it's a side computation, working on its
 * own local copies of 'row_values'/'c0' — except for 'column_pool'/
 * 'column_highest_cost'/'output.columns', which it updates the same way
 * regular pricing discoveries do, so newly found columns are available
 * for reuse by the real pricing loop too.
 */
struct RoundingHeuristicInput
{
    // Real column generation state, read-only.
    const Model& model;
    const ColumnGenerationParameters& parameters;
    const std::vector<Value>& row_values;
    Value c0;
    LinearProgrammingSolver* solver;
    const std::vector<std::shared_ptr<const Column>>& solver_columns;
    const std::vector<std::shared_ptr<const Cut>>& active_cuts;
    const std::vector<Value>& duals_out;
    const std::vector<std::pair<std::shared_ptr<const Cut>, Value>>& cut_duals;
    RowIdx number_of_rows;
    const std::vector<RowIdx>& new_row_indices;
    const std::vector<Value>& new_row_lower_bounds;
    const std::vector<Value>& new_row_upper_bounds;

    // Real column generation state this heuristic also updates.
    ColumnGenerationOutput& output;
    AlgorithmFormatter& algorithm_formatter;
    ColumnPool& column_pool;
    Value& column_highest_cost;

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
            Value v = (input.model.rows[element.row].upper_bound - row_values_tmp[element.row])
                / element.coefficient;
            value = (std::min)(value, v);
        } else if (element.coefficient < 0) {
            Value v = (row_values_tmp[element.row] - input.model.rows[element.row].lower_bound)
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
                input.model.rows[element.row].lower_bound,
                input.model.rows[element.row].upper_bound,
                row_values_tmp[element.row],
                input.row_violation_start[element.row]);
        row_values_tmp[element.row] += value * element.coefficient;
        infeasibility += rounding_heuristic_infeasibility_contribution(
                input.model.rows[element.row].lower_bound,
                input.model.rows[element.row].upper_bound,
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
    if (input.column_pool.find(column) != input.column_pool.end())
        return;
    input.column_pool.insert(column);
    Value value_max = std::numeric_limits<Value>::infinity();
    for (const LinearTerm& element: column->elements) {
        RowIdx new_row_id = input.new_row_indices[element.row];
        Value row_lower_bound = (new_row_id >= 0)?
            input.new_row_lower_bounds[new_row_id]:
            input.model.rows[element.row].lower_bound;
        Value row_upper_bound = (new_row_id >= 0)?
            input.new_row_upper_bounds[new_row_id]:
            input.model.rows[element.row].upper_bound;
        if (element.coefficient > 0) {
            Value v = row_upper_bound / element.coefficient;
            value_max = (std::min)(value_max, v);
        } else {
            Value v = row_lower_bound / element.coefficient;
            value_max = (std::min)(value_max, v);
        }
    }
    input.column_highest_cost = (std::max)(
            input.column_highest_cost,
            std::abs(column->objective_coefficient * value_max));
    input.output.columns.push_back(column);
}

void run_rounding_heuristic(RoundingHeuristicInput& input)
{
    auto start = std::chrono::high_resolution_clock::now();

    bool minimize = (input.model.objective_sense == optimizationtools::ObjectiveDirection::Minimize);
    input.has_incumbent = input.output.solution.feasible();
    Value incumbent = (input.has_incumbent)? input.output.solution.objective_value(): 0.0;
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
    input.row_violation_start.assign(input.number_of_rows, 0.0);
    for (RowIdx row_id = 0; row_id < input.number_of_rows; ++row_id) {
        input.row_violation_start[row_id] = rounding_heuristic_violation(
                input.model.rows[row_id].lower_bound,
                input.model.rows[row_id].upper_bound,
                input.row_values[row_id]);
    }
    input.objective_violation_start = (input.has_incumbent)?
        rounding_heuristic_violation(input.objective_lower_bound, input.objective_upper_bound, input.c0):
        0.0;

    Value initial_infeasibility = 0.0;
    for (RowIdx row_id = 0; row_id < input.number_of_rows; ++row_id) {
        initial_infeasibility += rounding_heuristic_infeasibility_contribution(
                input.model.rows[row_id].lower_bound,
                input.model.rows[row_id].upper_bound,
                input.row_values[row_id],
                input.row_violation_start[row_id]);
    }
    initial_infeasibility += rounding_heuristic_infeasibility_contribution(
            input.objective_lower_bound,
            input.objective_upper_bound,
            input.c0,
            input.objective_violation_start);

    if (initial_infeasibility > 0.0) {
        std::vector<Value> rh_row_values = input.row_values;
        Value rh_c0 = input.c0;
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
        for (const auto& p: relaxation_columns) {
            Value value = rounding_heuristic_max_value(input, p.first, rh_row_values, rh_c0);
            if (value <= 0.0)
                continue;
            rounding_heuristic_fix_column(input, p.first, value, rh_row_values, rh_c0, infeasibility);
            fixed_columns.push_back({p.first, value});

            if (infeasibility <= input.parameters.rounding_heuristic_infeasibility_threshold * initial_infeasibility) {
                threshold_reached = true;
                break;
            }
        }

        // Phase 2: complete the solution with a fix/price/fix loop, no
        // relaxation re-solve (mirrors the 'internal_diving' completion
        // loop in 'column_generation()'), only entered once Phase 1
        // resolved enough infeasibility.
        if (threshold_reached) {
            if (infeasibility > 0.0) {
                // Use the real current-iteration duals: they carry the
                // master LP's actual price signal, so pricing keeps
                // proposing columns that are genuinely attractive for the
                // relaxation, not just for the rows still short after
                // Phase 1's greedy fixing.
                for (;;) {
                    input.model.pricing_solver->initialize_pricing(fixed_columns, input.active_cuts, input.parameters.branching_decisions);
                    auto pricing_output = input.model.pricing_solver->solve_pricing(input.duals_out, input.cut_duals);
                    std::vector<std::shared_ptr<const Column>> new_columns;
                    for (const auto& column: pricing_output.columns) {
                        if (column->elements.empty())
                            continue;
                        new_columns.push_back(column);
                        rounding_heuristic_add_to_column_pool(input, column);
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
                                Value rc1 = input.model.compute_reduced_cost(*column_1, input.duals_out, input.cut_duals);
                                Value rc2 = input.model.compute_reduced_cost(*column_2, input.duals_out, input.cut_duals);
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
                // Restore the real pricing solver state for the pricing
                // calls in 'column_generation()'.
                input.model.pricing_solver->initialize_pricing(input.parameters.fixed_columns, input.active_cuts, input.parameters.branching_decisions);
            }

            // Build and check the candidate solution. Always done (rather
            // than gated on 'infeasibility <= 0.0') so 'Solution::
            // feasible()' — not this heuristic's own approximate
            // ratio-sum metric — is the sole authority on whether it's
            // reported.
            SolutionBuilder solution_builder;
            solution_builder.set_model(input.model);
            for (const auto& p: input.parameters.fixed_columns)
                solution_builder.add_column(p.first, p.second);
            for (const auto& p: fixed_columns)
                solution_builder.add_column(p.first, p.second);
            Solution solution = solution_builder.build();
            if (solution.feasible())
                input.algorithm_formatter.update_solution(solution);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    input.output.time_rounding_heuristic += time_span.count();
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

    if (parameters.dummy_column_objective_coefficient == 0) {
        throw std::invalid_argument(
                "columngenerationsolver::column_generation:"
                " input parameter 'dummy_column_objective_coefficient'"
                " must be non-null.");
    }

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
    Value column_highest_cost = 0;
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

        Value value_max = std::numeric_limits<Value>::infinity();
        for (const LinearTerm& element: column->elements) {
            // Use the residual row bounds (after subtracting the
            // contribution of already fixed columns) rather than the
            // original model bounds, otherwise the estimate can be wildly
            // overestimated once columns have been fixed.
            RowIdx new_row_id = new_row_indices[element.row];
            Value row_lower_bound = (new_row_id >= 0)?
                new_row_lower_bounds[new_row_id]:
                model.rows[element.row].lower_bound;
            Value row_upper_bound = (new_row_id >= 0)?
                new_row_upper_bounds[new_row_id]:
                model.rows[element.row].upper_bound;
            if (element.coefficient > 0) {
                Value v = row_upper_bound / element.coefficient;
                value_max = (std::min)(value_max, v);
            } else {
                Value v = row_lower_bound / element.coefficient;
                value_max = (std::min)(value_max, v);
            }
        }
        column_highest_cost = (std::max)(
                column_highest_cost,
                std::abs(column->objective_coefficient * value_max));
        column_pool.insert(column);
    }
    // Also account for the static columns: when the real objective lives
    // entirely on model.static_columns (e.g. columns fixed for the whole
    // model, outside of pricing), every generated column can have an
    // objective coefficient of 0 even though the problem isn't a pure
    // feasibility problem. Without this, 'column_highest_cost' would stay 0
    // and the dummy-column infeasibility check below would incorrectly fire
    // on ordinary column-generation degeneracy.
    for (const std::shared_ptr<const Column>& column: model.static_columns) {
        if (column->objective_coefficient == 0)
            continue;

        Value value_max = std::numeric_limits<Value>::infinity();
        for (const LinearTerm& element: column->elements) {
            RowIdx new_row_id = new_row_indices[element.row];
            Value row_lower_bound = (new_row_id >= 0)?
                new_row_lower_bounds[new_row_id]:
                model.rows[element.row].lower_bound;
            Value row_upper_bound = (new_row_id >= 0)?
                new_row_upper_bounds[new_row_id]:
                model.rows[element.row].upper_bound;
            if (element.coefficient > 0) {
                Value v = row_upper_bound / element.coefficient;
                value_max = (std::min)(value_max, v);
            } else {
                Value v = row_lower_bound / element.coefficient;
                value_max = (std::min)(value_max, v);
            }
        }
        column_highest_cost = (std::max)(
                column_highest_cost,
                std::abs(column->objective_coefficient * value_max));
    }

    Value overcost = (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
        -std::numeric_limits<Value>::infinity():
        +std::numeric_limits<Value>::infinity();

    // Active cuts. Starts from 'initial_cuts' and grows as cutting-plane
    // rounds find violated cuts below.
    std::vector<std::shared_ptr<const Cut>> active_cuts = parameters.initial_cuts;

    // Relaxation value at the point each cut was last removed for being
    // inactive. A cut in this list may only be removed again once the
    // relaxation has genuinely improved relative to the value recorded
    // here — otherwise a cut that gets removed, then found needed again,
    // then found inactive again without any real progress in between
    // would cycle indefinitely (remove, rebuild, re-add, rebuild, remove,
    // ...). Persists across cutting-plane rounds for the whole call,
    // unlike 'active_cuts' itself.
    //
    // Looked up by 'PricingSolver::equal' rather than by shared_ptr
    // identity or a hash map, since 'separate_cuts' may return a
    // different 'Cut' instance for the same constraint each time it
    // becomes violated again, and a custom equality without a matching
    // custom hash would break an unordered_map's bucket invariant. A
    // linear scan is fine given the expected number of active cuts.
    std::vector<std::pair<std::shared_ptr<const Cut>, Value>> cut_value_at_last_removal;

    // Loop for dummy columns.
    // If the final solution contains dummy columns, then the dummy column
    // objective value is increased and the algorithm is started again. The loop
    // is broken when the final solution doesn't contain any dummy column.
    output.dummy_column_objective_coefficient = parameters.dummy_column_objective_coefficient;
    std::vector<std::shared_ptr<const Column>> initial_columns = parameters.initial_columns;

    // Loop for cutting planes.
    // After the dummy-column loop below converges to a feasible relaxation
    // (no dummy column left), if cutting planes are enabled, cuts are
    // separated from that relaxation. If any are found, the whole master LP
    // is rebuilt from scratch (like a dummy-column retry) with the enlarged
    // cut set and re-optimized. The loop stops when no more violated cuts
    // are found, cutting planes are disabled, or the cutting-plane
    // iteration limit is reached.
    for (Counter cutting_plane_iteration = 0; ; ++cutting_plane_iteration) {

    // Compute residual cut bounds, after subtracting the contribution of
    // fixed columns (mirrors the row residual-bound computation above).
    std::vector<Value> new_cut_lower_bounds(active_cuts.size());
    std::vector<Value> new_cut_upper_bounds(active_cuts.size());
    for (CutIdx cut_pos = 0; cut_pos < (CutIdx)active_cuts.size(); ++cut_pos) {
        Value cut_fixed_value = 0.0;
        for (const auto& p: parameters.fixed_columns)
            cut_fixed_value += p.second * model.pricing_solver->coefficient(*active_cuts[cut_pos], *p.first);
        new_cut_lower_bounds[cut_pos] = active_cuts[cut_pos]->lower_bound - cut_fixed_value;
        new_cut_upper_bounds[cut_pos] = active_cuts[cut_pos]->upper_bound - cut_fixed_value;
    }

    // Appends the coefficients of 'column' in the active cuts to 'ri'/'rc',
    // at row indices following the model rows.
    auto append_cut_coefficients = [&model, &active_cuts, new_number_of_rows](
            const Column& column,
            std::vector<RowIdx>& ri,
            std::vector<Value>& rc)
    {
        for (CutIdx cut_pos = 0; cut_pos < (CutIdx)active_cuts.size(); ++cut_pos) {
            Value coef = model.pricing_solver->coefficient(*active_cuts[cut_pos], column);
            if (coef != 0.0) {
                ri.push_back(new_number_of_rows + cut_pos);
                rc.push_back(coef);
            }
        }
    };

    // Whether the dummy-column loop below converges to a feasible
    // relaxation (no dummy column), i.e. whether it is meaningful to
    // attempt cut separation afterwards.
    bool relaxation_is_feasible_for_cuts = false;

    for (;;) {
        //std::cout << "dummy_column_objective_coefficient " << output.dummy_column_objective_coefficient << std::endl;

        overcost = (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
            -std::numeric_limits<Value>::infinity():
            +std::numeric_limits<Value>::infinity();

        // Initialize solver
        //std::cout << "Initialize solver... " << parameters.solver_name << std::endl;
        std::vector<Value> lp_row_lower_bounds = new_row_lower_bounds;
        std::vector<Value> lp_row_upper_bounds = new_row_upper_bounds;
        lp_row_lower_bounds.insert(
                lp_row_lower_bounds.end(),
                new_cut_lower_bounds.begin(),
                new_cut_lower_bounds.end());
        lp_row_upper_bounds.insert(
                lp_row_upper_bounds.end(),
                new_cut_upper_bounds.begin(),
                new_cut_upper_bounds.end());

        std::unique_ptr<LinearProgrammingSolver> solver = NULL;
#if CPLEX_FOUND
        if (parameters.solver_name == SolverName::CPLEX)
            solver = std::unique_ptr<LinearProgrammingSolver>(
                    new LinearProgrammingSolverCplex(
                        model.objective_sense,
                        lp_row_lower_bounds,
                        lp_row_upper_bounds));
#endif
#if CLP_FOUND
        if (parameters.solver_name == SolverName::CLP) {
            solver = std::unique_ptr<LinearProgrammingSolver>(
                    new LinearProgrammingSolverClp(
                        model.objective_sense,
                        lp_row_lower_bounds,
                        lp_row_upper_bounds));
        }
#endif
#if HIGHS_FOUND
        if (parameters.solver_name == SolverName::Highs) {
            solver = std::unique_ptr<LinearProgrammingSolver>(
                    new LinearProgrammingSolverHighs(
                        model.objective_sense,
                        lp_row_lower_bounds,
                        lp_row_upper_bounds));
        }
#endif
#if XPRESS_FOUND
        if (parameters.solver_name == SolverName::Xpress) {
            solver = std::unique_ptr<LinearProgrammingSolver>(
                    new LinearProgrammingSolverXpress(
                        model.objective_sense,
                        lp_row_lower_bounds,
                        lp_row_upper_bounds));
        }
#endif
#if KNITRO_FOUND
        if (parameters.solver_name == SolverName::Knitro)
            solver = std::unique_ptr<LinearProgrammingSolver>(
                    new LinearProgrammingSolverKnitro(
                        model.objective_sense,
                        lp_row_lower_bounds,
                        lp_row_upper_bounds));
#endif
        if (solver == NULL) {
            throw std::runtime_error("ERROR, no linear programming solver found");
        }

        // This array is used to retrieve the corresponding column from a
        // variable id in the LP solver solution.
        std::vector<std::shared_ptr<const Column>> solver_columns;

        // We never add a generated column more than once in the LP solver.
        // We use this set to keep track of the generated columns inside the
        // LP solver.
        std::unordered_set<std::shared_ptr<const Column>> solver_generated_columns;

        output.number_of_columns_in_linear_subproblem = 0;

        // Initialize pricing solver.
        //std::cout << "Initialize pricing solver..." << std::endl;
        std::vector<std::shared_ptr<const Column>> infeasible_columns
            = model.pricing_solver->initialize_pricing(parameters.fixed_columns, active_cuts, parameters.branching_decisions);
        std::vector<int8_t> feasible(model.static_columns.size(), 1);

        // Add dummy columns.
        std::vector<RowIdx> dummy_column_rows;
        for (RowIdx row_id = 0; row_id < new_number_of_rows; ++row_id) {
            if (new_row_lower_bounds[row_id] > 0) {
                solver_columns.push_back(nullptr);
                solver->add_column(
                        {row_id},
                        {new_row_lower_bounds[row_id]},
                        (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        +output.dummy_column_objective_coefficient:
                        -output.dummy_column_objective_coefficient,
                        0,
                        std::numeric_limits<Value>::infinity());
                output.number_of_columns_in_linear_subproblem++;
                dummy_column_rows.push_back(row_id);
            }
            if (new_row_upper_bounds[row_id] < 0) {
                solver_columns.push_back(nullptr);
                solver->add_column(
                        {row_id},
                        {new_row_upper_bounds[row_id]},
                        (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        +output.dummy_column_objective_coefficient:
                        -output.dummy_column_objective_coefficient,
                        0,
                        std::numeric_limits<Value>::infinity());
                output.number_of_columns_in_linear_subproblem++;
                dummy_column_rows.push_back(row_id);
            }
        }
        // Add dummy columns for cut rows that fixed/static/initial columns
        // alone cannot satisfy (symmetric to the model-row dummy columns
        // above).
        for (CutIdx cut_pos = 0; cut_pos < (CutIdx)active_cuts.size(); ++cut_pos) {
            RowIdx cut_row_id = new_number_of_rows + cut_pos;
            if (new_cut_lower_bounds[cut_pos] > 0) {
                solver_columns.push_back(nullptr);
                solver->add_column(
                        {cut_row_id},
                        {new_cut_lower_bounds[cut_pos]},
                        (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        +output.dummy_column_objective_coefficient:
                        -output.dummy_column_objective_coefficient,
                        0,
                        std::numeric_limits<Value>::infinity());
                output.number_of_columns_in_linear_subproblem++;
                dummy_column_rows.push_back(cut_row_id);
            }
            if (new_cut_upper_bounds[cut_pos] < 0) {
                solver_columns.push_back(nullptr);
                solver->add_column(
                        {cut_row_id},
                        {new_cut_upper_bounds[cut_pos]},
                        (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        +output.dummy_column_objective_coefficient:
                        -output.dummy_column_objective_coefficient,
                        0,
                        std::numeric_limits<Value>::infinity());
                output.number_of_columns_in_linear_subproblem++;
                dummy_column_rows.push_back(cut_row_id);
            }
        }

        // Add model columns.
        std::vector<Value> lower_bounds;
        std::vector<Value> upper_bounds;
        std::vector<Value> objective_coefficients;
        std::vector<std::vector<RowIdx>> row_ids;
        std::vector<std::vector<Value>> row_coefficients;
        for (const std::shared_ptr<const Column>& column: model.static_columns) {
            model.check_column(column);

            // Don't add the column if it has already been fixed.
            bool is_fixed = false;
            for (const auto& p: parameters.fixed_columns)
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
                if (model.rows[element.row].coefficient_lower_bound >= 0
                        && column->type == VariableType::Integer
                        && row_values[element.row] + element.coefficient
                        > model.rows[element.row].upper_bound) {
                    //if (print) {
                    //    std::cout << "element " << element.row
                    //        << " " << element.coefficient
                    //        << std::endl;
                    //}
                    ok = false;
                    break;
                }
                if (new_row_indices[element.row] < 0) {
                    ok = false;
                    break;
                }
                ri.push_back(new_row_indices[element.row]);
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
            objective_coefficients.push_back(column->objective_coefficient);
            row_ids.push_back(ri);
            row_coefficients.push_back(rc);
            output.number_of_columns_in_linear_subproblem++;
        }
        solver->add_columns(
                row_ids,
                row_coefficients,
                objective_coefficients,
                lower_bounds,
                upper_bounds);

        // Add initial columns.
        for (const std::shared_ptr<const Column>& column: initial_columns) {
            model.check_generated_column(column);

            // Check column feasibility.
            if (std::find(infeasible_columns.begin(), infeasible_columns.end(), column)
                    != infeasible_columns.end())
                continue;

            // Don't add a tabu column.
            if (parameters.tabu != nullptr
                    && parameters.tabu->find(column) != parameters.tabu->end())
                continue;

            std::vector<RowIdx> row_ids;
            std::vector<Value> row_coefficients;
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
                row_ids.push_back(new_row_indices[element.row]);
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
                    column->objective_coefficient,
                    0,
                    std::numeric_limits<Value>::infinity());
            output.number_of_columns_in_linear_subproblem++;
        }

        // Duals given to the pricing solver.
        std::vector<Value> duals_sep(number_of_rows, 0);
        // π_in, duals at the previous point.
        std::vector<Value> duals_in(number_of_rows, 0);
        // π_out, duals of next point without stabilization.
        std::vector<Value> duals_out(number_of_rows, 0);
        // π_in + (1 − α) (π_out − π_in)
        std::vector<Value> duals_tilde(number_of_rows, 0);
        // Duals in the direction of the subgradient.
        std::vector<Value> duals_g(number_of_rows, 0);
        // β π_g + (1 − β) π_out
        std::vector<Value> rho(number_of_rows, 0);
        std::vector<Value> lagrangian_constraint_values(number_of_rows, 0);
        // g_in.
        std::vector<Value> subgradient(number_of_rows, 0);
        // Cut duals (not stabilized: the active cut set is fixed for the
        // whole CG loop, so there is no smoothing history to maintain).
        // Paired with the cut itself (like 'fixed_columns' already is),
        // so 'solve_pricing'/'compute_reduced_cost' callers don't have to
        // separately track 'active_cuts' just to correlate the two.
        std::vector<std::pair<std::shared_ptr<const Cut>, Value>> cut_duals;
        cut_duals.reserve(active_cuts.size());
        for (const std::shared_ptr<const Cut>& cut: active_cuts)
            cut_duals.push_back({cut, 0.0});
        double alpha = parameters.static_wentges_smoothing_parameter;

        RoundingHeuristicInput rounding_heuristic_input{
                model,
                parameters,
                row_values,
                c0,
                solver.get(),
                solver_columns,
                active_cuts,
                duals_out,
                cut_duals,
                number_of_rows,
                new_row_indices,
                new_row_lower_bounds,
                new_row_upper_bounds,
                output,
                algorithm_formatter,
                column_pool,
                column_highest_cost};

        for (Counter number_of_column_generation_iterations = 1;
                ;
                ++number_of_column_generation_iterations) {
            //std::cout << "number_of_column_generation_iterations " << number_of_column_generation_iterations << std::endl;

            // Solve LP
            auto start_lpsolve = std::chrono::high_resolution_clock::now();
            solver->solve();
            auto end_lpsolve = std::chrono::high_resolution_clock::now();
            auto time_span_lpsolve = std::chrono::duration_cast<std::chrono::duration<double>>(end_lpsolve - start_lpsolve);
            output.time_lpsolve += time_span_lpsolve.count();
            output.relaxation_solution_value = c0 + solver->objective();

            // The bound and the per-iteration display are computed after
            // pricing below, once 'overcost' reflects a reduced cost
            // computed at the same duals as this 'relaxation_solution_value'
            // (rather than the previous iteration's duals) — see there.
            output.number_of_column_generation_iterations++;

            // Check time.
            if (parameters.timer.needs_to_end())
                break;
            // Check iteration limit.
            if (parameters.maximum_number_of_iterations != -1
                    && output.number_of_column_generation_iterations
                    >= parameters.maximum_number_of_iterations) {
                break;
            }

            // Get duals from linear programming solver.
            for (RowIdx row_pos = 0; row_pos < new_number_of_rows; ++row_pos) {
                duals_out[new_rows[row_pos]] = solver->dual(row_pos);
            }
            for (CutIdx cut_pos = 0; cut_pos < (CutIdx)active_cuts.size(); ++cut_pos) {
                cut_duals[cut_pos].second = solver->dual(new_number_of_rows + cut_pos);
            }

            if (parameters.rounding_heuristic)
                run_rounding_heuristic(rounding_heuristic_input);

            std::vector<std::shared_ptr<const Column>> new_columns;
            std::vector<Value> pricing_lagrangian_column_values;

            // Search for new columns from the column pool.
            for (const std::shared_ptr<const Column>& column: column_pool) {

                // Don't add a column which is already in the LP.
                if (solver_generated_columns.find(column) != solver_generated_columns.end())
                    continue;

                // Don't add a tabu column.
                if (parameters.tabu != nullptr
                        && parameters.tabu->find(column) != parameters.tabu->end())
                    continue;

                // Add the column if its reduced cost is negative.
                Value rc = model.compute_reduced_cost(*column, duals_out, cut_duals);
                if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                        && rc < -parameters.optimality_tolerance) {
                    new_columns.push_back(column);
                }
                if (model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                        && rc > parameters.optimality_tolerance) {
                    new_columns.push_back(column);
                }

            }

            if (new_columns.empty()) {
                // Search for new columns by solving the pricing problem.

                duals_in = duals_sep; // The last shall be the first.
                //std::cout << "alpha " << alpha << std::endl;
                for (Counter k = 1; ; ++k) {
                    // Mispricing number.

                    // Update global mispricing number.
                    if (k > 1)
                        output.number_of_mispricings++;

                    // Compute separation point.
                    double alpha_cur = std::max(0.0, 1 - k * (1 - alpha) - FFOT_TOL);
                    double beta = parameters.static_directional_smoothing_parameter;
                    //std::cout << "alpha_cur " << alpha_cur << std::endl;
                    if (number_of_column_generation_iterations == 1
                            || norm(new_rows, subgradient) == 0
                            // Shouldn't happen, but happens with Cplex.
                            || norm(new_rows, duals_in, duals_out) == 0
                            || k > 1
                            // No directional smoothing.
                            || (!parameters.automatic_directional_smoothing && beta == 0)) {

                        //std::cout << "compute duals_sep..." << std::endl;
                        for (RowIdx row_id: new_rows) {
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
                        for (RowIdx row_id: new_rows) {
                            duals_tilde[row_id]
                                = alpha_cur * duals_in[row_id]
                                + (1 - alpha_cur) * duals_out[row_id];
                        }

                        // Compute π_g.
                        //std::cout << "compute duals_g..." << std::endl;
                        Value coef_g
                            = norm(new_rows, duals_in, duals_out)
                            / norm(new_rows, subgradient);
                        for (RowIdx row_id: new_rows) {
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
                        if (parameters.automatic_directional_smoothing) {
                            Value dot_product = 0;
                            for (RowIdx row_id: new_rows) {
                                dot_product
                                    += (duals_out[row_id] - duals_in[row_id])
                                    * (duals_g[row_id] - duals_in[row_id]);
                            }
                            beta = dot_product
                                / norm(new_rows, duals_in, duals_out)
                                / norm(new_rows, duals_in, duals_g);
                            //std::cout << "beta " << beta << std::endl;
                            //assert(beta >= 0);
                            beta = std::max(0.0, std::min(1.0, beta));
                        }

                        // Compute ρ.
                        //std::cout << "compute rho..." << std::endl;
                        for (RowIdx row_id: new_rows) {
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
                        //std::cout << "norm(new_rows, duals_in, duals_tilde) " << norm(new_rows, duals_in, duals_tilde) << std::endl;
                        //std::cout << "norm(new_rows, duals_in, rho) " << norm(new_rows, duals_in, rho) << std::endl;
                        Value norm_rho = norm(new_rows, duals_in, rho);
                        if (norm_rho < FFOT_TOL) {
                            // ρ ≈ π_in: directional adjustment is undefined;
                            // fall back to plain Wentges smoothing.
                            for (RowIdx row_id: new_rows)
                                duals_sep[row_id] = duals_tilde[row_id];
                        } else {
                            Value coef_sep
                                = norm(new_rows, duals_in, duals_tilde)
                                / norm_rho;
                            for (RowIdx row_id: new_rows) {
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
                    if (!parameters.internal_diving) {
                        auto pricing_output = model.pricing_solver->solve_pricing(duals_sep, cut_duals);
                        all_columns = pricing_output.columns;
                        overcost = pricing_output.overcost;
                        pricing_lagrangian_column_values = std::move(pricing_output.lagrangian_column_values);
                        for (const auto& column: all_columns)
                            model.check_generated_column(column);
                    } else {
                        std::vector<Value> row_values_tmp = row_values;
                        std::vector<std::pair<std::shared_ptr<const Column>, Value>> fixed_columns_tmp = parameters.fixed_columns;
                        for (int i = 0;; ++i) {
                            model.pricing_solver->initialize_pricing(fixed_columns_tmp, active_cuts, parameters.branching_decisions);
                            auto pricing_output = model.pricing_solver->solve_pricing(duals_sep, cut_duals);
                            std::vector<std::shared_ptr<const Column>> all_columns_tmp_0
                                = pricing_output.columns;
                            if (i == 0) {
                                overcost = pricing_output.overcost;
                                pricing_lagrangian_column_values = std::move(pricing_output.lagrangian_column_values);
                            }
                            for (const auto& column: all_columns_tmp_0)
                                model.check_generated_column(column);
                            std::vector<std::shared_ptr<const Column>> all_columns_tmp_1;
                            for (const auto& column: all_columns_tmp_0) {
                                if (column->elements.empty())
                                    continue;
                                all_columns_tmp_1.push_back(column);
                                all_columns.push_back(column);
                            }
                            if (all_columns_tmp_1.empty())
                                break;

                            // Sort new columns by reduced cost.
                            std::sort(
                                    all_columns_tmp_1.begin(),
                                    all_columns_tmp_1.end(),
                                    [&model, &duals_out, &cut_duals](
                                        const std::shared_ptr<const Column>& column_1,
                                        const std::shared_ptr<const Column>& column_2)
                                    {
                                        Value rc1 = model.compute_reduced_cost(*column_1, duals_out, cut_duals);
                                        Value rc2 = model.compute_reduced_cost(*column_2, duals_out, cut_duals);
                                        if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize) {
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
                                            = (model.rows[element.row].upper_bound
                                                    - row_values_tmp[element.row])
                                            / element.coefficient;
                                        value = (std::min)(value, std::floor(v));
                                    } else {
                                        Value v
                                            = (row_values_tmp[element.row]
                                                    - model.rows[element.row].lower_bound)
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
                        model.pricing_solver->initialize_pricing(parameters.fixed_columns, active_cuts, parameters.branching_decisions);
                    }

                    auto end_pricing = std::chrono::high_resolution_clock::now();
                    auto time_span_pricing = std::chrono::duration_cast<std::chrono::duration<double>>(end_pricing - start_pricing);
                    output.time_pricing += time_span_pricing.count();
                    output.number_of_pricings++;
                    if (alpha_cur == 0 && beta == 0)
                        output.number_of_no_stab_pricings++;

                    // Look for negative reduced cost columns.
                    for (const std::shared_ptr<const Column>& column: all_columns) {

                        // Discard columns which have already been generated.
                        // If they were worth adding to the LP, then they would
                        // have been added at the previous step (looking for
                        // column from the pool).
                        if (column_pool.find(column) != column_pool.end())
                            continue;

                        // Store these new columns.
                        column_pool.insert(column);
                        Value value_max = std::numeric_limits<Value>::infinity();
                        for (const LinearTerm& element: column->elements) {
                            // Use the residual row bounds (after subtracting
                            // the contribution of already fixed columns)
                            // rather than the original model bounds,
                            // otherwise the estimate can be wildly
                            // overestimated once columns have been fixed.
                            RowIdx new_row_id = new_row_indices[element.row];
                            Value row_lower_bound = (new_row_id >= 0)?
                                new_row_lower_bounds[new_row_id]:
                                model.rows[element.row].lower_bound;
                            Value row_upper_bound = (new_row_id >= 0)?
                                new_row_upper_bounds[new_row_id]:
                                model.rows[element.row].upper_bound;
                            if (element.coefficient > 0) {
                                Value v = row_upper_bound / element.coefficient;
                                value_max = (std::min)(value_max, v);
                            } else {
                                Value v = row_lower_bound / element.coefficient;
                                value_max = (std::min)(value_max, v);
                            }
                        }
                        column_highest_cost = (std::max)(
                                column_highest_cost,
                                std::abs(column->objective_coefficient * value_max));
                      output.columns.push_back(column);

                      // Only add the ones with negative reduced cost.
                      Value rc = model.compute_reduced_cost(*column, duals_out, cut_duals);
                      // std::cout << "rc " << rc << std::endl;
                      if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                              && rc < -parameters.optimality_tolerance)
                        new_columns.push_back(column);
                      if (model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                              && rc > parameters.optimality_tolerance)
                        new_columns.push_back(column);
                    }

                    if (!new_columns.empty() || (alpha_cur == 0.0 && beta == 0.0)) {
                        if (k == 1)
                            output.number_of_first_try_pricings++;
                        break;
                    }

                }

            }

            // Update bound and display this iteration, now that 'overcost'
            // reflects a reduced cost computed at the same duals
            // ('duals_out') as 'output.relaxation_solution_value' above —
            // giving the tightest bound achievable from this iteration's
            // master solve, rather than the previous iteration's (still
            // valid, since 'relaxation_solution_value' only improves across
            // iterations, but needlessly loose).
            Value bound = (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                -std::numeric_limits<Value>::infinity():
                +std::numeric_limits<Value>::infinity();
            if (overcost != std::numeric_limits<Value>::infinity()) {
                bound = output.relaxation_solution_value + overcost;
            }
            algorithm_formatter.update_bound(bound);
            algorithm_formatter.print_column_generation_iteration(
                    output.number_of_column_generation_iterations,
                    output.number_of_columns_in_linear_subproblem,
                    output.relaxation_solution_value,
                    output.bound);
            parameters.iteration_callback(output);

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
            for (RowIdx row_id = 0; row_id < new_number_of_rows; ++row_id) {
                subgradient[new_rows[row_id]]
                    = std::min(
                            0.0,
                            new_row_upper_bounds[row_id]
                            - lagrangian_constraint_values[new_rows[row_id]])
                    + std::max(
                            0.0,
                            new_row_lower_bounds[row_id]
                            - lagrangian_constraint_values[new_rows[row_id]]);
                //std::cout << " row " << row_id
                //    << " lb " << new_row_lower_bounds[row_id]
                //    << " ub " << new_row_upper_bounds[row_id]
                //    << " val " << lagrangian_constraint_values[new_rows[row_id]]
                //    << std::endl;
            }

            // Adjust alpha.
            if (parameters.self_adjusting_wentges_smoothing
                    && norm(new_rows, duals_in, duals_sep) != 0) {
                //for (RowIdx i: new_rows)
                //    std::cout
                //        << "i " << i
                //        << " y " << lagrangian_constraint_values[i]
                //        << " dual_in " << duals_in[i]
                //        << " dual_out " << duals_out[i]
                //        << " dual_sep " << duals_sep[i]
                //        << " diff " << duals_sep[i] - duals_in[i]
                //        << " l " << new_row_lower_bounds[i]
                //        << " u " << new_row_upper_bounds[i]
                //        << " g " << subgradient[i]
                //        << std::endl;

                // Compute g^sep · (π^out - π^in) per Pessoa et al. (2018),
                // Section 4. A positive dot product means the subgradient at
                // the sep-point is aligned with the direction toward π^out, so
                // a larger step would improve the dual bound: decrease α (less
                // smoothing). Note: Table 1 of the paper has f_incr/f_decr
                // swapped in Step 4; the body text on p. 347 is correct.
                Value v = 0;
                for (RowIdx row_id: new_rows)
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
            for (const std::shared_ptr<const Column>& column: new_columns) {

                //std::cout << column << std::endl;
                std::vector<RowIdx> ri;
                std::vector<Value> rc;
                for (RowIdx row_pos = 0;
                        row_pos < (RowIdx)column->elements.size();
                        ++row_pos) {
                    RowIdx i = column->elements[row_pos].row;
                    Value c = column->elements[row_pos].coefficient;
                    if (new_row_indices[i] < 0) {
                        throw std::logic_error("");
                    }
                    ri.push_back(new_row_indices[i]);
                    rc.push_back(c);
                }
                append_cut_coefficients(*column, ri, rc);
                solver_columns.push_back(column);
                solver_generated_columns.insert(column);
                solver->add_column(
                        ri,
                        rc,
                        column->objective_coefficient,
                        0,
                        std::numeric_limits<double>::infinity());
                output.number_of_columns_in_linear_subproblem++;
            }
        }

        // Compute relaxation solution.
        SolutionBuilder solution_builder;
        solution_builder.set_model(model);
        for (const auto& p: parameters.fixed_columns) {
            solution_builder.add_column(
                    p.first,
                    p.second);
        }
        bool has_dummy_column = false;
        for (ColIdx column_id = 0;
                column_id < (ColIdx)solver_columns.size();
                ++column_id) {
            if (std::abs(solver->primal(column_id)) < FFOT_TOL)
                continue;
            if (solver_columns[column_id] == nullptr) {
                has_dummy_column = true;
                //RowIdx row_orig_id = new_rows[dummy_column_rows[column_id]];
                //std::cout << "dummy column id " << column_id
                //    << " row_lp " << dummy_column_rows[column_id]
                //    << " row_orig " << row_orig_id
                //    << " name " << model.rows[row_orig_id].name
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
            }
        }

        // Check time.
        if (parameters.timer.needs_to_end()) {
            output.relaxation_solution = solution_builder.build();
            output.cuts = active_cuts;
            algorithm_formatter.end();
            return output;
        }
        // Check iteration limit.
        if (parameters.maximum_number_of_iterations != -1
                && output.number_of_column_generation_iterations
                > parameters.maximum_number_of_iterations) {
            output.relaxation_solution = solution_builder.build();
            output.cuts = active_cuts;
            algorithm_formatter.end();
            return output;
        }

        // If the final solution doesn't contain any dummy column, then stop.
        if (!has_dummy_column) {
            output.relaxation_solution_is_feasible = true;
            //std::cout << "feasible" << std::endl;
            output.relaxation_solution = solution_builder.build();
            if (!output.relaxation_solution.feasible_relaxation()) {
                throw std::logic_error(
                        "columngenerationsolver::column_generation: "
                        "infeasible relaxation solution.");
            }
            relaxation_is_feasible_for_cuts = true;
            break;
        }

        // If the final solution contains some dummy columns, and the dummy
        // column objective coefficient is significantly larger than the
        // largest generated column objective coefficient, then we consider the
        // problem infeasible.
        // No 'column_highest_cost > 0' guard here: when every column has an
        // objective coefficient of 0 (a pure feasibility problem),
        // 'column_highest_cost' is always 0, so the threshold below already
        // reduces to 'dummy coefficient > 0', which is the correct
        // condition in that case (the master LP's optimal basis doesn't
        // depend on the dummy coefficient's magnitude when the real
        // objective is identically 0, only on whether it's positive).
        if (std::abs(output.dummy_column_objective_coefficient)
                > 100 * column_highest_cost) {
            //std::cout << "infeasible" << std::endl;
            output.relaxation_solution_is_feasible = false;
            algorithm_formatter.update_bound(
                    (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        std::numeric_limits<Value>::infinity():
                        -std::numeric_limits<Value>::infinity());
            output.relaxation_solution = solution_builder.build();
            break;
        }
        if (parameters.tabu != nullptr
                && parameters.tabu->size() > 0) {
            output.relaxation_solution_is_feasible = false;
            output.relaxation_solution = solution_builder.build();
            break;
        }

        // Otherwise, increase the dummy column objective coefficient and
        // restart.
        output.dummy_column_objective_coefficient *= 4;
        // Use current solution as initial columns of the next loop.
        initial_columns = parameters.initial_columns;
        for (const auto& p: output.relaxation_solution.columns())
            if (column_pool.find(p.first) != column_pool.end())
                initial_columns.push_back(p.first);
    }

    // If the dummy-column loop above didn't converge to a feasible
    // relaxation (timeout/iteration-limit already returned directly;
    // infeasible or tabu fell through to here), stop: no point separating
    // cuts from a relaxation that still contains dummy columns.
    if (!relaxation_is_feasible_for_cuts)
        break;

    // Cutting planes disabled for this call: stop after the first feasible
    // relaxation, exactly like before cuts existed.
    if (!parameters.cutting_planes)
        break;

    // Check cutting-plane iteration limit.
    if (parameters.maximum_number_of_cutting_plane_iterations != -1
            && cutting_plane_iteration >= parameters.maximum_number_of_cutting_plane_iterations) {
        break;
    }

    // Separate cuts from the current (feasible) relaxation solution.
    std::vector<std::shared_ptr<const Cut>> new_cuts
        = model.pricing_solver->separate_cuts(output.relaxation_solution);

    // Remove cuts that are no longer active: their value at the current
    // relaxation solution has slack on both sides, more than their own
    // 'feasibility_tolerance', relative to their bounds (the same check
    // 'Row::feasibility_tolerance' does for rows). Checking the value
    // rather than the dual avoids a false "inactive" reading under LP
    // degeneracy — common in exactly the set-partitioning-style
    // formulations this framework targets — where a constraint can be
    // geometrically at its bound yet still be reported with a zero dual,
    // because multiple dual solutions can correspond to the same primal
    // optimum. Non-robust cuts especially can make pricing significantly
    // harder, so don't keep paying for ones that aren't helping.
    //
    // A cut that has already been removed once may only be removed again
    // if the relaxation has genuinely improved since then (sense-aware,
    // guarded by FFOT_TOL against numerical noise) — otherwise a cut that
    // gets removed, found needed again, then found inactive again without
    // any real progress in between would cycle indefinitely. This still
    // lets a cut be removed multiple times over a long search, as long as
    // each removal is preceded by real progress, rather than forbidding
    // it outright after a single bounce-back.
    //
    // 'output.cuts' (see 'ColumnGenerationOutput::cuts') mirrors
    // 'active_cuts' by the time this call returns, so a cut dropped here —
    // whether it came in via 'parameters.initial_cuts' or was newly separated
    // this call — is dropped from 'output.cuts' too, and a caller feeding
    // 'output.cuts' into a follow-up call won't keep reinstating it.
    //
    // 'PricingSolver::equal' is only called once a cut has already been
    // removed at least once this call (i.e. 'cut_value_at_last_removal' is
    // non-empty), and only for cuts that are themselves candidates for
    // removal — so a 'PricingSolver' that never triggers a removal, or
    // doesn't use cuts at all, never needs to implement it.
    bool minimize = (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize);
    Value current_value = output.relaxation_solution.objective_value();
    std::vector<std::shared_ptr<const Cut>> still_active_cuts;
    bool removed_a_cut = false;
    for (const std::shared_ptr<const Cut>& cut: active_cuts) {
        Value cut_value = 0.0;
        for (const auto& p: output.relaxation_solution.columns())
            cut_value += p.second * model.pricing_solver->coefficient(*cut, *p.first);

        bool has_slack_below = (cut_value > cut->lower_bound + cut->feasibility_tolerance);
        bool has_slack_above = (cut_value < cut->upper_bound - cut->feasibility_tolerance);
        bool eligible_for_removal = has_slack_below && has_slack_above;

        auto previous_removal = cut_value_at_last_removal.end();
        if (eligible_for_removal && !cut_value_at_last_removal.empty()) {
            previous_removal = std::find_if(
                    cut_value_at_last_removal.begin(),
                    cut_value_at_last_removal.end(),
                    [&model, &cut](
                        const std::pair<std::shared_ptr<const Cut>, Value>& p)
                    {
                        return model.pricing_solver->equal(*cut, *p.first);
                    });
            if (previous_removal != cut_value_at_last_removal.end()) {
                eligible_for_removal = (minimize)?
                    (current_value < previous_removal->second - FFOT_TOL):
                    (current_value > previous_removal->second + FFOT_TOL);
            }
        }

        if (eligible_for_removal) {
            removed_a_cut = true;
            if (previous_removal != cut_value_at_last_removal.end()) {
                previous_removal->second = current_value;
            } else {
                cut_value_at_last_removal.push_back({cut, current_value});
            }
        } else {
            still_active_cuts.push_back(cut);
        }
    }
    active_cuts = std::move(still_active_cuts);

    if (new_cuts.empty() && !removed_a_cut)
        break;

    active_cuts.insert(active_cuts.end(), new_cuts.begin(), new_cuts.end());
    output.number_of_cutting_plane_iterations++;

    // Rebuild the master LP from scratch with the enlarged cut set: reset
    // the dummy column coefficient (the previous round's inflated value has
    // no bearing on the new LP) and seed initial columns with the columns
    // of the relaxation solution that just converged, same as a
    // dummy-column retry already does.
    output.dummy_column_objective_coefficient = parameters.dummy_column_objective_coefficient;
    initial_columns = parameters.initial_columns;
    for (const auto& p: output.relaxation_solution.columns())
        if (column_pool.find(p.first) != column_pool.end())
            initial_columns.push_back(p.first);
    }

    // Update bound. A no-op for the common case (the per-iteration update
    // above already reported this exact bound right before the loop
    // converged), but still needed as a safety net for the timer/iteration-
    // limit-hit-before-pricing case, where the loop broke before that
    // iteration ever reached pricing, so no per-iteration update happened
    // for it.
    Value bound = (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
        -std::numeric_limits<Value>::infinity():
        +std::numeric_limits<Value>::infinity();
    if (overcost != std::numeric_limits<Value>::infinity()) {
        bound = output.relaxation_solution_value + overcost;
    }
    algorithm_formatter.update_bound(bound);

    output.cuts = active_cuts;
    algorithm_formatter.end();
    return output;
}
