#include "columngenerationsolver/algorithms/column_generation.hpp"

#include "columngenerationsolver/algorithm_formatter.hpp"

#include <unordered_set>

using namespace columngenerationsolver;

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
    //std::cout << "fixed_columns.size() " << fixed_columns.size() << std::endl;

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
                && row_values[row_id] > model.rows[row_id].upper_bound) {
            // TODO improve float comparison.
            // Infeasible.
            return output;
        }
        if (model.rows[row_id].coefficient_lower_bound >= 0
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
            if (element.coefficient > 0) {
                Value v = model.rows[element.row].upper_bound / element.coefficient;
                value_max = (std::min)(value_max, v);
            } else {
                Value v = model.rows[element.row].lower_bound / element.coefficient;
                value_max = (std::min)(value_max, v);
            }
        }
        column_highest_cost = (std::max)(
                column_highest_cost,
                std::abs(column->objective_coefficient * value_max));
        column_pool.insert(column);
    }

    // Loop for dummy columns.
    // If the final solution contains dummy columns, then the dummy column
    // objective value is increased and the algorithm is started again. The loop
    // is broken when the final solution doesn't contain any dummy column.
    output.dummy_column_objective_coefficient = parameters.dummy_column_objective_coefficient;
    std::vector<std::shared_ptr<const Column>> initial_columns = parameters.initial_columns;
    for (;;) {
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
        if (parameters.linear_programming_solver == LinearProgrammingSolver::CLP) {
            solver = std::unique_ptr<ColumnGenerationSolver>(
                    new ColumnGenerationSolverClp(
                        model.objective_sense,
                        new_row_lower_bounds,
                        new_row_upper_bounds));
        }
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
            = model.pricing_solver->initialize_pricing(parameters.fixed_columns);
        std::vector<int8_t> feasible(model.columns.size(), 1);

        // Add dummy columns.
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
                        +output.dummy_column_objective_coefficient:
                        -output.dummy_column_objective_coefficient,
                        0,
                        std::numeric_limits<Value>::infinity());
                output.number_of_columns_in_linear_subproblem++;
            }
        }

        // Add model columns.
        for (const std::shared_ptr<const Column>& column: model.columns) {
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

            std::vector<RowIdx> row_ids;
            std::vector<Value> row_coefficients;
            bool ok = true;
            for (const LinearTerm& element: column->elements) {
                // The column might not be feasible.
                // For example, it corresponds to the same bin / machine that a
                // currently fixed column or it contains an item / job also
                // included in a currently fixed column.
                if (element.coefficient >= 0
                        && column->type == VariableType::Integer
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
            solver->add_column(
                    row_ids,
                    row_coefficients,
                    column->objective_coefficient,
                    column->lower_bound,
                    column->upper_bound);
            output.number_of_columns_in_linear_subproblem++;
        }

        // Add initial columns.
        for (const std::shared_ptr<const Column>& column: initial_columns) {
            model.check_generated_column(column);

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
                if (element.coefficient >= 0
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
        double alpha = parameters.static_wentges_smoothing_parameter;
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

            // Display.
            algorithm_formatter.print_column_generation_iteration(
                    output.number_of_column_generation_iterations,
                    output.number_of_columns_in_linear_subproblem,
                    output.relaxation_solution_value);
            parameters.iteration_callback(output);
            output.number_of_column_generation_iterations++;

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

            std::vector<std::shared_ptr<const Column>> new_columns;

            // Search for new columns from the column pool.
            for (const std::shared_ptr<const Column>& column: column_pool) {

                // Don't add a column which is already in the LP.
                if (solver_generated_columns.find(column) != solver_generated_columns.end())
                    continue;

                // Add the column if its reduced cost is negative.
                Value rc = compute_reduced_cost(*column, duals_out);
                if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                        && rc < 0) {
                    new_columns.push_back(column);
                }
                if (model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                        && rc > 0) {
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
                            beta = std::max(0.0, beta);
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
                        Value coef_sep
                            = norm(new_rows, duals_in, duals_tilde)
                            / norm(new_rows, duals_in, rho);
                        //std::cout << "norm(new_rows, duals_in, duals_tilde) " << norm(new_rows, duals_in, duals_tilde) << std::endl;
                        //std::cout << "norm(new_rows, duals_in, rho) " << norm(new_rows, duals_in, rho) << std::endl;
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

                    // Call pricing solver on the computed separation point.
                    auto start_pricing = std::chrono::high_resolution_clock::now();

                    std::vector<std::shared_ptr<const Column>> all_columns;
                    if (!parameters.internal_diving) {
                        all_columns = model.pricing_solver->solve_pricing(duals_sep);
                        for (const auto& column: all_columns)
                            model.check_generated_column(column);
                    } else {
                        std::vector<Value> row_values_tmp = row_values;
                        std::vector<std::pair<std::shared_ptr<const Column>, Value>> fixed_columns_tmp = parameters.fixed_columns;
                        for (;;) {
                            model.pricing_solver->initialize_pricing(fixed_columns_tmp);
                            std::vector<std::shared_ptr<const Column>> all_columns_tmp_0
                                = model.pricing_solver->solve_pricing(duals_sep);
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
                            if (element.coefficient > 0) {
                                Value v = model.rows[element.row].upper_bound / element.coefficient;
                                value_max = (std::min)(value_max, v);
                            } else {
                                Value v = model.rows[element.row].lower_bound / element.coefficient;
                                value_max = (std::min)(value_max, v);
                            }
                        }
                        column_highest_cost = (std::max)(
                                column_highest_cost,
                                std::abs(column->objective_coefficient * value_max));
                      output.columns.push_back(column);

                      // Only add the ones with negative reduced cost.
                      Value rc = compute_reduced_cost(*column, duals_out);
                      // std::cout << "rc " << rc << std::endl;
                      if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                              && rc < 0)
                        new_columns.push_back(column);
                      if (model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                              && rc > 0)
                        new_columns.push_back(column);
                    }

                    if (!new_columns.empty() || (alpha_cur == 0.0 && beta == 0.0)) {
                        if (k == 1)
                            output.number_of_first_try_pricings++;
                        break;
                    }

                }

            }

            // Stop the column generation procedure if no negative reduced cost
            // column has been found.
            //std::cout << "new_columns.size() " << new_columns.size() << std::endl;
            if (new_columns.empty())
                break;

            // Get lagrangian constraint values.
            std::fill(
                    lagrangian_constraint_values.begin(),
                    lagrangian_constraint_values.end(),
                    0);
            for (const std::shared_ptr<const Column>& column: new_columns)
                for (const LinearTerm& element: column->elements)
                    lagrangian_constraint_values[element.row] += element.coefficient;

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

                // It seems to work with this minus '-', but I don't undersstand
                // why.
                Value v = 0;
                for (RowIdx row_id: new_rows)
                    v += subgradient[row_id] * (duals_sep[row_id] - duals_in[row_id]);
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
                    if (new_row_indices[i] < 0)
                        continue;
                    ri.push_back(new_row_indices[i]);
                    rc.push_back(c);
                }
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

        // If the final solution doesn't contain any dummy column, then stop.
        if (!has_dummy_column) {
            output.relaxation_solution = solution_builder.build();
            break;
        }

        // If the final solution contains some dummy columns, and the dummy
        // column objective coefficient is significantly larger than the
        // largest generated column objective coefficient, then we consider the
        // problem infeasible.
        if (column_highest_cost > 0
                && std::abs(output.dummy_column_objective_coefficient)
                > 100 * column_highest_cost) {
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

    // Update bound.
    algorithm_formatter.update_bound(output.relaxation_solution.objective_value());

    algorithm_formatter.end();
    return output;
}
