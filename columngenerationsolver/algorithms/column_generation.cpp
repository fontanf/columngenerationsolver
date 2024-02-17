#include "columngenerationsolver/algorithms/column_generation.hpp"

#include "columngenerationsolver/algorithm_formatter.hpp"

#include "optimizationtools/containers/indexed_map.hpp"

#include <unordered_set>

using namespace columngenerationsolver;

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
        if (column_1->lower_bound
                != column_2->lower_bound)
            return false;
        if (column_1->upper_bound
                != column_2->upper_bound)
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
        optimizationtools::hash_combine(hash, hasher_value(column->lower_bound));
        optimizationtools::hash_combine(hash, hasher_value(column->upper_bound));
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
    //std::cout << "fixed_columns.size() " << fixed_columns.size() << std::endl;

    ColumnHasher column_hasher(model);
    std::unordered_set<std::shared_ptr<const Column>,
                       const ColumnHasher&,
                       const ColumnHasher&> column_pool(0, column_hasher, column_hasher);
    for (const auto& column: parameters.column_pool)
        column_pool.insert(column);

    // Compute row values.
    //std::cout << "Compute row values..." << std::endl;
    std::vector<Value> row_values(number_of_rows, 0.0);
    Value c0 = 0.0;

    for (auto p: parameters.fixed_columns) {
        const Column& column = *(p.first);
        Value value = p.second;
        for (const LinearTerm& element: column.elements)
            row_values[element.row] += value * element.coefficient;
        c0 += value * column.objective_coefficient;
    }

    // Compute fixed rows;
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
        if (model.column_lower_bound >= 0
                && model.rows[row_id].coefficient_lower_bound >= 0
                && row_values[row_id] > model.rows[row_id].upper_bound) {
            // TODO improve float comparison.
            // Infeasible.
            return output;
        }
        if (model.column_lower_bound >= 0
                && model.rows[row_id].coefficient_lower_bound >= 0
                && row_values[row_id] == model.rows[row_id].upper_bound) {
            // TODO improve float comparison.
            //std::cout
            //    << "row " << row
            //    << " ub " << model.row_upper_bounds[row]
            //    << " val " << row_values[row]
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

    // Loop for dummy columns.
    // If the final solution contains dummy columns, then the dummy column
    // objective value is increased and the algorithm is started again. The loop
    // is broken when the final solution doesn't contain any dummy column.
    output.dummy_column_objective_coefficient = parameters.dummy_column_objective_coefficient;
    std::vector<std::shared_ptr<const Column>> initial_columns = parameters.initial_columns;
    for (;;) {
        bool has_dummy_column = true;
        //std::cout << "dummy_column_objective_coefficient " << output.dummy_column_objective_coefficient << std::endl;

        // Initialize solver
        //std::cout << "Initialize solver..." << std::endl;
        std::unique_ptr<ColumnGenerationSolver> solver = NULL;
#if CPLEX_FOUND
        if (parameters.linear_programming_solver == LinearProgrammingSolver::CPLEX)
            solver = std::unique_ptr<ColumnGenerationSolver>(
                    new ColumnGenerationSolverCplex(
                        model.objective_sense,
                        new_row_lower_bounds,
                        new_row_upper_bounds));
#endif
#if CLP_FOUND
        if (parameters.linear_programming_solver == LinearProgrammingSolver::CLP) {
            solver = std::unique_ptr<ColumnGenerationSolver>(
                    new ColumnGenerationSolverClp(
                        model.objective_sense,
                        new_row_lower_bounds,
                        new_row_upper_bounds));
        }
#endif
#if XPRESS_FOUND
        if (parameters.linear_programming_solver == LinearProgrammingSolver::Xpress) {
            solver = std::unique_ptr<ColumnGenerationSolver>(
                    new ColumnGenerationSolverXpress(
                        model.objective_sense,
                        new_row_lower_bounds,
                        new_row_upper_bounds));
        }
#endif
#if KNITRO_FOUND
        if (parameters.linear_programming_solver == LinearProgrammingSolver::Knitro)
            solver = std::unique_ptr<ColumnGenerationSolver>(
                    new ColumnGenerationSolverKnitro(
                        model.objective_sense,
                        new_row_lower_bounds,
                        new_row_upper_bounds));
#endif
        if (solver == NULL) {
            throw std::runtime_error("ERROR, no linear programming solver found");
        }

        output.number_of_columns_in_linear_subproblem = 0;
        std::vector<std::shared_ptr<const Column>> solver_columns;
        std::unordered_set<std::shared_ptr<const Column>,
            const ColumnHasher&,
            const ColumnHasher&> solver_columns_set(0, column_hasher, column_hasher);

        // Add dummy columns.
        //std::cout << "Add dumm columns..." << std::endl;
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
            }
            if (new_row_upper_bounds[row_id] < 0) {
                solver_columns.push_back(nullptr);
                solver->add_column(
                        {row_id},
                        {new_row_upper_bounds[row_id]},
                        (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                        -output.dummy_column_objective_coefficient:
                        +output.dummy_column_objective_coefficient,
                        0,
                        std::numeric_limits<Value>::infinity());
                output.number_of_columns_in_linear_subproblem++;
            }
        }

        // Initialize pricing solver.
        //std::cout << "Initialize pricing solver..." << std::endl;
        std::vector<std::shared_ptr<const Column>> infeasible_columns
            = model.pricing_solver->initialize_pricing(parameters.fixed_columns);
        std::vector<int8_t> feasible(model.columns.size(), 1);

        // Add initial columns.
        //std::cout << "Add initial columns..." << std::endl;
        for (const auto& columns: {model.columns, initial_columns}) {
            for (const std::shared_ptr<const Column>& column: columns) {

                // Check column feasibility.
                if (std::find(infeasible_columns.begin(), infeasible_columns.end(), column)
                        != infeasible_columns.end())
                    continue;

                std::vector<RowIdx> row_ids;
                std::vector<Value> row_coefficients;
                bool ok = true;
                for (const LinearTerm& element: column->elements) {
                    // The column might not be feasible.
                    // For example, it corresponds to the same bin / machine that a
                    // currently fixed column or it contains an item / job also
                    // included in a currently fixed column.
                    if (model.column_lower_bound >= 0
                            && element.coefficient >= 0
                            && row_values[element.row] + element.coefficient
                            > model.rows[element.row].upper_bound) {
                        ok = false;
                        break;
                    }
                    if (new_row_indices[element.row] < 0)
                        continue;
                    row_ids.push_back(new_row_indices[element.row]);
                    row_coefficients.push_back(element.coefficient);
                }
                if (!ok)
                    continue;
                solver_columns.push_back(column);
                solver_columns_set.insert(column);
                solver->add_column(
                        row_ids,
                        row_coefficients,
                        column->objective_coefficient,
                        model.column_lower_bound,
                        std::numeric_limits<Value>::infinity());
                output.number_of_columns_in_linear_subproblem++;
            }
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
        double alpha = parameters.static_wentges_smoothing_parameter;
        for (output.number_of_column_generation_iterations = 1;
                ;
                output.number_of_column_generation_iterations++) {

            // Solve LP
            auto start_lpsolve = std::chrono::high_resolution_clock::now();
            solver->solve();
            auto end_lpsolve = std::chrono::high_resolution_clock::now();
            auto time_span_lpsolve = std::chrono::duration_cast<std::chrono::duration<double>>(end_lpsolve - start_lpsolve);
            output.time_lpsolve += time_span_lpsolve.count();

            // Compute relaxation solution
            SolutionBuilder solution_builder;
            solution_builder.set_model(model);
            has_dummy_column = false;
            for (ColIdx column_id = 0;
                    column_id < (ColIdx)solver_columns.size();
                    ++column_id) {
                if (std::abs(solver->primal(column_id)) < FFOT_TOL)
                    continue;
                if (solver_columns[column_id] == nullptr) {
                    has_dummy_column = true;
                } else {
                    solution_builder.add_column(
                            solver_columns[column_id],
                            solver->primal(column_id));
                }
            }
            output.relaxation_solution = solution_builder.build();
            output.relaxation_solution_value = c0 + solver->objective();

            // Display.
            algorithm_formatter.print_column_generation_iteration(
                    output.number_of_column_generation_iterations,
                    output.number_of_columns_in_linear_subproblem,
                    output.relaxation_solution_value);
            parameters.iteration_callback(output);

            // Check time.
            if (parameters.timer.needs_to_end())
                break;
            // Check iteration limit.
            if (parameters.maximum_number_of_iterations != -1
                    && output.number_of_column_generation_iterations
                    > parameters.maximum_number_of_iterations) {
                break;
            }

            // Get duals from linear programming solver.
            for (RowIdx row_pos = 0; row_pos < new_number_of_rows; ++row_pos) {
                duals_out[new_rows[row_pos]] = solver->dual(row_pos);
            }

            // Search for new columns from the column pool.
            std::vector<std::shared_ptr<const Column>> new_columns;
            for (const std::shared_ptr<const Column>& column: parameters.column_pool) {
                if (solver_columns_set.find(column) != solver_columns_set.end())
                    continue;
                bool ok = true;
                for (const LinearTerm& element: column->elements) {
                    // The column might not be feasible.
                    // For example, it corresponds to the same bin / machine that a
                    // currently fixed column or it contains an item / job also
                    // included in a currently fixed column.
                    if (model.column_lower_bound >= 0
                            && element.coefficient >= 0
                            && row_values[element.row] + element.coefficient
                            > model.rows[element.row].upper_bound) {
                        ok = false;
                        break;
                    }
                    if (new_row_indices[element.row] < 0)
                        continue;
                }
                if (!ok)
                    continue;
                Value rc = compute_reduced_cost(*column, duals_out);
                if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                        && rc <= 0) {
                    //std::cout << "rc " << rc
                    //    << " " << -model.dummy_column_objective_coefficient * 1e-9
                    //    << std::endl;
                    new_columns.push_back(column);
                }
                if (model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                        && rc >= 0) {
                    new_columns.push_back(column);
                }
            }

            if (new_columns.empty()) {
                // Search for new columns by solving the pricing problem.

                duals_in = duals_sep; // The last shall be the first.
                                      //std::cout << "alpha " << alpha << std::endl;
                for (Counter k = 1; ; ++k) { // Mispricing number.
                                             // Update global mispricing number.
                    if (k > 1)
                        output.number_of_mispricings++;
                    // Compute separation point.
                    double alpha_cur = std::max(0.0, 1 - k * (1 - alpha) - FFOT_TOL);
                    double beta = parameters.static_directional_smoothing_parameter;
                    //std::cout << "alpha_cur " << alpha_cur << std::endl;
                    if (output.number_of_column_generation_iterations == 1
                            || norm(new_rows, duals_in, duals_out) == 0 // Shouldn't happen, but happens with Cplex.
                            || k > 1
                            || (!parameters.automatic_directional_smoothing && beta == 0)) { // No directional smoothing.
                        for (RowIdx row_id: new_rows) {
                            duals_sep[row_id]
                                = alpha_cur * duals_in[row_id]
                                + (1 - alpha_cur) * duals_out[row_id];
                        }
                        //for (RowIdx i: new_rows)
                        //    std::cout
                        //        << "i " << i
                        //        << " in " << duals_in[i]
                        //        << " sep " << duals_sep[i] << std::endl;
                    } else { // Directional smoothing.
                             // Compute π_tilde.
                        for (RowIdx row_id: new_rows) {
                            duals_tilde[row_id]
                                = alpha_cur * duals_in[row_id]
                                + (1 - alpha_cur) * duals_out[row_id];
                        }
                        // Compute π_g.
                        Value coef_g
                            = norm(new_rows, duals_in, duals_out)
                            / norm(new_rows, subgradient);
                        for (RowIdx row_id: new_rows) {
                            duals_g[row_id]
                                = duals_in[row_id]
                                + coef_g * subgradient[row_id];
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
                            beta = std::max(0.0, beta);
                        }
                        // Compute ρ.
                        for (RowIdx row_id: new_rows) {
                            rho[row_id]
                                = beta * duals_g[row_id]
                                + (1 - beta) * duals_out[row_id];
                        }
                        // Compute π_sep.
                        Value coef_sep
                            = norm(new_rows, duals_in, duals_tilde)
                            / norm(new_rows, duals_in, rho);
                        for (RowIdx row_id: new_rows) {
                            duals_sep[row_id]
                                = duals_in[row_id]
                                + coef_sep * (rho[row_id] - duals_in[row_id]);
                        }
                        //for (RowIdx i: new_rows)
                        //    std::cout
                        //        << "i " << i
                        //        << " in " << duals_in[i]
                        //        << " out " << duals_out[i]
                        //        << " sg " << subgradient[i]
                        //        << " g " << duals_g[i]
                        //        << " tilde " << duals_tilde[i]
                        //        << " rho " << rho[i]
                        //        << " sep " << duals_sep[i] << std::endl;
                    }

                    // Call pricing solver on the computed separation point.
                    auto start_pricing = std::chrono::high_resolution_clock::now();

                    std::vector<std::shared_ptr<const Column>> all_columns_tmp;
                    if (!parameters.internal_diving) {
                        all_columns_tmp = model.pricing_solver->solve_pricing(duals_sep);
                    } else {
                        std::vector<Value> row_values_tmp = row_values;
                        std::vector<std::pair<std::shared_ptr<const Column>, Value>> fixed_columns_tmp = parameters.fixed_columns;
                        for (;;) {
                            model.pricing_solver->initialize_pricing(fixed_columns_tmp);
                            std::vector<std::shared_ptr<const Column>> all_columns_tmp_0
                                = model.pricing_solver->solve_pricing(duals_sep);
                            std::vector<std::shared_ptr<const Column>> all_columns_tmp_1;
                            for (const auto& column: all_columns_tmp_0) {
                                if (column->elements.empty())
                                    continue;
                                all_columns_tmp_1.push_back(column);
                                all_columns_tmp.push_back(column);
                            }
                            if (all_columns_tmp_1.empty())
                                break;
                            // Sort new columns by reduced cost.
                            std::sort(
                                    all_columns_tmp.begin(),
                                    all_columns_tmp.end(),
                                    [&model, &duals_out](
                                        const std::shared_ptr<const Column>& column_1,
                                        const std::shared_ptr<const Column>& column_2)
                                    {
                                    Value rc1 = compute_reduced_cost(*column_1, duals_out);
                                    Value rc2 = compute_reduced_cost(*column_2, duals_out);
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
                                        value = (std::min)(value, v);
                                    } else {
                                        Value v
                                            = (row_values_tmp[element.row]
                                                    - model.rows[element.row].lower_bound)
                                            / (-element.coefficient);
                                        value = (std::min)(value, v);
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
                        model.pricing_solver->initialize_pricing(parameters.fixed_columns);
                    }
                    std::vector<std::shared_ptr<const Column>> all_columns;
                    for (const auto& column: all_columns_tmp) {
                        if (column_pool.find(column) != column_pool.end())
                            continue;
                        column_pool.insert(column);
                        all_columns.push_back(column);
                        output.columns.push_back(column);
                    }

                    auto end_pricing = std::chrono::high_resolution_clock::now();
                    auto time_span_pricing = std::chrono::duration_cast<std::chrono::duration<double>>(end_pricing - start_pricing);
                    output.time_pricing += time_span_pricing.count();
                    output.number_of_pricings++;
                    if (alpha_cur == 0 && beta == 0)
                        output.number_of_no_stab_pricings++;

                    // Look for negative reduced cost columns.
                    for (const std::shared_ptr<const Column>& column: all_columns) {
                        if (solver_columns_set.find(column) != solver_columns_set.end())
                            continue;
                        Value rc = compute_reduced_cost(*column, duals_out);
                        //std::cout << "rc " << rc
                        //    << " " << -model.dummy_column_objective_coefficient * 1e-9
                        //    << std::endl;
                        if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                                && rc <= -parameters.dummy_column_objective_coefficient * 1e-9)
                            new_columns.push_back(column);
                        if (model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                                && rc >= -parameters.dummy_column_objective_coefficient * 1e-9)
                            new_columns.push_back(column);
                    }
                    if (!new_columns.empty() || (alpha_cur == 0.0 && beta == 0.0)) {
                        if (k == 1)
                            output.number_of_first_try_pricings++;
                        break;
                    }
                }

                // Stop the column generation procedure if no negative reduced cost
                // column has been found.
                //std::cout << "new_columns.size() " << new_columns.size() << std::endl;
                if (new_columns.empty())
                    break;

            }

            // Get lagrangian constraint values.
            std::fill(
                    lagrangian_constraint_values.begin(),
                    lagrangian_constraint_values.end(),
                    0);
            for (const std::shared_ptr<const Column>& column: new_columns) {
                for (RowIdx row_pos = 0;
                        row_pos < (RowIdx)column->elements.size();
                        ++row_pos) {
                    RowIdx row_id = column->elements[row_pos].row;
                    Value row_coefficient = column->elements[row_pos].coefficient;
                    lagrangian_constraint_values[row_id] += row_coefficient;
                }
            }
            // Compute subgradient at separation point.
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
                Value v = 0;
                // It seems to work with this minus '-', but I don't undersstand
                // why.
                for (RowIdx i: new_rows)
                    v += subgradient[i] * (duals_sep[i] - duals_in[i]);
                //std::cout << "v " << v << std::endl;
                // Update alpha.
                if (v > 0) {
                    alpha = std::max(0.0, alpha - 0.1);
                } else {
                    alpha = std::min(0.99, alpha + (1.0 - alpha) * 0.1);
                }
            }

            for (const std::shared_ptr<const Column>& column: new_columns) {
                //std::cout << column << std::endl;
                // Add new column to the local LP solver.
                std::vector<RowIdx> ri;
                std::vector<Value> rc;
                for (RowIdx row_pos = 0;
                        row_pos < (RowIdx)column->elements.size();
                        ++row_pos) {
                    RowIdx i = column->elements[row_pos].row;
                    Value c = column->elements[row_pos].coefficient;
                    if (new_row_indices[i] < 0)
                        continue;
                    ri.push_back(new_row_indices[i]);
                    rc.push_back(c);
                }
                solver_columns.push_back(column);
                solver_columns_set.insert(column);
                solver->add_column(
                        ri,
                        rc,
                        column->objective_coefficient,
                        model.column_lower_bound,
                        std::numeric_limits<double>::infinity());
                output.number_of_columns_in_linear_subproblem++;
            }
        }

        // If the final solution doesn't contain any dummy column, then stop.
        if (!has_dummy_column)
            break;
        // Otherwise, increase the dummy column objective coefficient and
        // restart.
        output.dummy_column_objective_coefficient *= 4;
        // Use current solution as initial columns of the next loop.
        initial_columns.clear();
        for (const auto& p: output.relaxation_solution.columns())
            initial_columns.push_back(p.first);
    }

    // Update bound.
    algorithm_formatter.update_bound(output.relaxation_solution.objective_value());

    algorithm_formatter.end();
    return output;
}
