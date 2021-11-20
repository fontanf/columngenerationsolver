#pragma once

#include "columngenerationsolver/linear_programming_solver.hpp"

namespace columngenerationsolver
{

struct ColumnGenerationOptionalParameters
{
    LinearProgrammingSolver linear_programming_solver = LinearProgrammingSolver::CLP;

    const std::vector<std::pair<ColIdx, Value>>* fixed_columns = NULL;

    Counter maximum_number_of_iterations = -1;
    bool* end = NULL;

    // Stabilization parameters.
    Value static_wentges_smoothing_parameter = 0; // alpha
    bool self_adjusting_wentges_smoothing = false;
    Value static_directional_smoothing_parameter = 0.0; // beta
    bool automatic_directional_smoothing = false;

    optimizationtools::Info info = optimizationtools::Info();
};

struct ColumnGenerationOutput
{
    std::vector<std::pair<ColIdx, Value>> solution;
    Value solution_value;
    Counter number_of_iterations = 0;
    Counter number_of_added_columns = 0;
    Counter number_of_good_pricings = 0;
    Counter number_of_pricings = 0;
    Counter number_of_first_try_pricings = 0;
    Counter number_of_mispricings = 0;
    Counter number_of_no_stab_pricings = 0;
    double time_pricing = 0.0;
    double time_lpsolve = 0.0;
};

inline ColumnGenerationOutput column_generation(
        Parameters& parameters,
        ColumnGenerationOptionalParameters optional_parameters = {})
{
    // Initial display.
    VER(optional_parameters.info,
               "======================================" << std::endl
            << "       Column Generation Solver       " << std::endl
            << "======================================" << std::endl
            << std::endl
            << "Algorithm" << std::endl
            << "---------" << std::endl
            << "Column Generation" << std::endl
            << std::endl
            << "Parameters" << std::endl
            << "----------" << std::endl
            << "Linear programming solver:               " << optional_parameters.linear_programming_solver << std::endl
            << "Static Wentges smoothing parameter:      " << optional_parameters.static_wentges_smoothing_parameter << std::endl
            << "Static directional smoothing parameter:  " << optional_parameters.static_directional_smoothing_parameter << std::endl
            << "Self-adjusting Wentges smoothing:        " << optional_parameters.self_adjusting_wentges_smoothing << std::endl
            << "Automatic directional smoothing:         " << optional_parameters.automatic_directional_smoothing << std::endl
            << "Maximum number of iterations:            " << optional_parameters.maximum_number_of_iterations << std::endl
            << std::endl
            << std::setw(10) << "Time" << std::setw(10) << "It" << std::setw(20) << "Obj" << std::setw(10) << "Col" << std::endl
            << std::setw(10) << "----" << std::setw(10) << "--" << std::setw(20) << "---" << std::setw(10) << "---" << std::endl
       );

    ColumnGenerationOutput output;

    RowIdx m = parameters.row_lower_bounds.size();
    //std::cout << "m " << m << std::endl;
    //std::cout << "fixed_columns.size() " << fixed_columns.size() << std::endl;

    // Compute row values.
    //std::cout << "Compute row values..." << std::endl;
    std::vector<RowIdx> row_values(m, 0.0);
    Value c0 = 0.0;

    const std::vector<std::pair<ColIdx, Value>> fixed_columns_default;
    const std::vector<std::pair<ColIdx, Value>>* fixed_columns
        = (optional_parameters.fixed_columns != NULL)?
        optional_parameters.fixed_columns: &fixed_columns_default;
    for (auto p: *fixed_columns) {
        ColIdx col = p.first;
        Value val = p.second;
        const Column& column = parameters.columns[col];
        for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
            RowIdx row_index = column.row_indices[row_pos];
            Value row_coefficient = column.row_coefficients[row_pos];
            row_values[row_index] += val * row_coefficient;
            //std::cout << val << " " << row_coefficient << " " << val * row_coefficient << std::endl;
        }
        c0 += column.objective_coefficient;
    }

    // Compute fixed rows;
    //std::cout << "Compute fixed rows..." << std::endl;
    std::vector<RowIdx> new_row_indices(m, -2);
    std::vector<RowIdx> new_rows;
    RowIdx row_pos = 0;
    for (RowIdx row = 0; row < m; ++row) {
        //std::cout
        //    << "row " << row
        //    << " lb " << parameters.row_lower_bounds[row]
        //    << " val " << row_values[row]
        //    << " ub " << parameters.row_upper_bounds[row]
        //    << std::endl;
        if (parameters.column_lower_bound >= 0
                && parameters.row_coefficient_lower_bounds[row] >= 0
                && row_values[row] > parameters.row_upper_bounds[row]) {
            // Infeasible.
            return output;
        }
        if (parameters.column_lower_bound >= 0
                && parameters.row_coefficient_lower_bounds[row] >= 0
                && row_values[row] == parameters.row_upper_bounds[row]) {
            //std::cout
            //    << "row " << row
            //    << " ub " << parameters.row_upper_bounds[row]
            //    << " val " << row_values[row]
            //    << std::endl;
            continue;
        }
        new_row_indices[row] = row_pos;
        new_rows.push_back(row);
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
    for (RowIdx row = 0; row < new_number_of_rows; ++row) {
        new_row_lower_bounds[row] = parameters.row_lower_bounds[new_rows[row]] - row_values[new_rows[row]];
        new_row_upper_bounds[row] = parameters.row_upper_bounds[new_rows[row]] - row_values[new_rows[row]];
        //std::cout << "row " << row << " lb " << new_row_lower_bounds[row] << " ub " << new_row_upper_bounds[row] << std::endl;
    }

    // Initialize solver
    //std::cout << "Initialize solver..." << std::endl;
    std::unique_ptr<ColumnGenerationSolver> solver = NULL;
#if CPLEX_FOUND
    if (optional_parameters.linear_programming_solver == LinearProgrammingSolver::CPLEX)
        solver = std::unique_ptr<ColumnGenerationSolver>(
                new ColumnGenerationSolverCplex(
                    parameters.objective_sense,
                    new_row_lower_bounds,
                    new_row_upper_bounds));
#endif
#if COINOR_FOUND
    if (optional_parameters.linear_programming_solver == LinearProgrammingSolver::CLP)
        solver = std::unique_ptr<ColumnGenerationSolver>(
                new ColumnGenerationSolverClp(
                    parameters.objective_sense,
                    new_row_lower_bounds,
                    new_row_upper_bounds));
#endif
#if KNITRO_FOUND
    if (optional_parameters.linear_programming_solver == LinearProgrammingSolver::Knitro)
        solver = std::unique_ptr<ColumnGenerationSolver>(
                new ColumnGenerationSolverKnitro(
                    parameters.objective_sense,
                    new_row_lower_bounds,
                    new_row_upper_bounds));
#endif
    if (solver == NULL) {
        std::cerr << "\033[31m" << "ERROR, no linear programming solver found." << "\033[0m" << std::endl;
        assert(false);
        return output;
    }

    std::vector<ColIdx> solver_column_indices;

    // Add dummy columns.
    //std::cout << "Add dumm columns..." << std::endl;
    RowIdx number_of_dummy_columns = 0;
    for (RowIdx row = 0; row < new_number_of_rows; ++row) {
        if (new_row_lower_bounds[row] > 0) {
            solver_column_indices.push_back(-1);
            number_of_dummy_columns++;
            solver->add_column(
                    {row},
                    {new_row_lower_bounds[row]},
                    parameters.dummy_column_objective_coefficient,
                    parameters.column_lower_bound,
                    parameters.column_upper_bound);
        }
        if (new_row_upper_bounds[row] < 0) {
            solver_column_indices.push_back(-1);
            number_of_dummy_columns++;
            solver->add_column(
                    {row},
                    {new_row_upper_bounds[row]},
                    parameters.dummy_column_objective_coefficient,
                    parameters.column_lower_bound,
                    parameters.column_upper_bound);
        }
    }

    // Initialize pricing solver.
    //std::cout << "Initialize pricing solver..." << std::endl;
    std::vector<ColIdx> infeasible_columns = parameters.pricing_solver->initialize_pricing(parameters.columns, *fixed_columns);
    std::vector<int8_t> feasible(parameters.columns.size(), 1);
    for (ColIdx col: infeasible_columns)
        feasible[col] = 0;

    // Add initial columns.
    //std::cout << "Add initial columns..." << std::endl;
    for (ColIdx col = 0; col < (ColIdx)parameters.columns.size(); ++col) {
        if (!feasible[col])
            continue;
        const Column& column = parameters.columns[col];
        std::vector<RowIdx> ri(new_number_of_rows);
        std::vector<Value> rc(new_number_of_rows);
        bool ok = true;
        for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
            RowIdx i = column.row_indices[row_pos];
            Value c = column.row_coefficients[row_pos];
            // The column might not be feasible.
            // For example, it corresponds to the same bin / machine that a
            // currently fixed column or it contains an item / job also
            // included in a currently fixed column.
            if (parameters.column_lower_bound >= 0
                    && c >= 0
                    && row_values[i] + c > parameters.row_upper_bounds[i]) {
                ok = false;
                break;
            }
            if (new_row_indices[i] < 0)
                continue;
            ri.push_back(new_row_indices[i]);
            rc.push_back(c);
        }
        if (!ok)
            continue;
        solver_column_indices.push_back(col);
        solver->add_column(
                ri,
                rc,
                column.objective_coefficient,
                parameters.column_lower_bound,
                parameters.column_upper_bound);
    }

    std::vector<Value> duals_sep(m, 0); // Duals given to the pricing solver.
    std::vector<Value> duals_in(m, 0); // π_in, duals at the previous point.
    std::vector<Value> duals_out(m, 0); // π_out, duals of next point without stabilization.
    std::vector<Value> duals_tilde(m, 0); // π_in + (1 − α) (π_out − π_in)
    std::vector<Value> duals_g(m, 0); // Duals in the direction of the subgradient.
    std::vector<Value> rho(m, 0); // β π_g + (1 − β) π_out
    std::vector<Value> lagrangian_constraint_values(m, 0);
    std::vector<Value> subgradient(m, 0); // g_in.
    double alpha = optional_parameters.static_wentges_smoothing_parameter;
    for (output.number_of_iterations = 1;; output.number_of_iterations++) {
        // Solve LP
        auto start_lpsolve = std::chrono::high_resolution_clock::now();
        solver->solve();
        auto end_lpsolve = std::chrono::high_resolution_clock::now();
        auto time_span_lpsolve = std::chrono::duration_cast<std::chrono::duration<double>>(end_lpsolve - start_lpsolve);
        output.time_lpsolve += time_span_lpsolve.count();

        // Display.
        VER(optional_parameters.info,
                std::setw(10) << std::fixed << std::setprecision(3) << optional_parameters.info.elapsed_time()
                << std::setw(10) << output.number_of_iterations
                << std::setw(20) << std::fixed << std::setprecision(7) << c0 + solver->objective()
                << std::setw(10) << output.number_of_added_columns
                << std::endl);

        // Check time.
        if (optional_parameters.info.needs_to_end())
            break;
        // Check 'end' parameter.
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;
        // Check iteration limit.
        if (optional_parameters.maximum_number_of_iterations != -1
                && output.number_of_iterations > optional_parameters.maximum_number_of_iterations)
            break;

        // Search for new columns.
        std::vector<Column> all_columns;
        std::vector<Column> new_columns;
        duals_in = duals_sep; // The last shall be the first.
        // Get duals from linear programming solver.
        for (RowIdx row_pos = 0; row_pos < new_number_of_rows; ++row_pos)
            duals_out[new_rows[row_pos]] = solver->dual(row_pos);
        //std::cout << "alpha " << alpha << std::endl;
        for (Counter k = 1; ; ++k) { // Mispricing number.
            // Update global mispricing number.
            if (k > 1)
                output.number_of_mispricings++;
            // Compute separation point.
            double alpha_cur = std::max(0.0, 1 - k * (1 - alpha) - TOL);
            double beta = optional_parameters.static_directional_smoothing_parameter;
            //std::cout << "alpha_cur " << alpha_cur << std::endl;
            if (output.number_of_iterations == 1
                    || norm(new_rows, duals_in, duals_out) == 0 // Shouldn't happen, but happens with Cplex.
                    || k > 1
                    || (!optional_parameters.automatic_directional_smoothing && beta == 0)) { // No directional smoothing.
                for (RowIdx i: new_rows)
                    duals_sep[i]
                        = alpha_cur * duals_in[i]
                        + (1 - alpha_cur) * duals_out[i];
                //for (RowIdx i: new_rows)
                //    std::cout
                //        << "i " << i
                //        << " in " << duals_in[i]
                //        << " sep " << duals_sep[i] << std::endl;
            } else { // Directional smoothing.
                // Compute π_tilde.
                for (RowIdx i: new_rows)
                    duals_tilde[i]
                        = alpha_cur * duals_in[i]
                        + (1 - alpha_cur) * duals_out[i];
                // Compute π_g.
                Value coef_g
                    = norm(new_rows, duals_in, duals_out)
                    / norm(new_rows, subgradient);
                for (RowIdx i: new_rows)
                    duals_g[i]
                        = duals_in[i]
                        + coef_g * subgradient[i];
                // Compute β.
                if (optional_parameters.automatic_directional_smoothing) {
                    Value dot_product = 0;
                    for (RowIdx i: new_rows)
                        dot_product += (duals_out[i] - duals_in[i]) * (duals_g[i] - duals_in[i]);
                    beta = dot_product
                            / norm(new_rows, duals_in, duals_out)
                            / norm(new_rows, duals_in, duals_g);
                    //std::cout << "beta " << beta << std::endl;
                    //assert(beta >= 0);
                    beta = std::max(0.0, beta);
                }
                // Compute ρ.
                for (RowIdx i: new_rows)
                    rho[i]
                        = beta * duals_g[i]
                        + (1 - beta) * duals_out[i];
                // Compute π_sep.
                Value coef_sep
                    = norm(new_rows, duals_in, duals_tilde)
                    / norm(new_rows, duals_in, rho);
                for (RowIdx i: new_rows)
                    duals_sep[i]
                        = duals_in[i]
                        + coef_sep * (rho[i] - duals_in[i]);
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
            all_columns = parameters.pricing_solver->solve_pricing(duals_sep);
            auto end_pricing = std::chrono::high_resolution_clock::now();
            auto time_span_pricing = std::chrono::duration_cast<std::chrono::duration<double>>(end_pricing - start_pricing);
            output.time_pricing += time_span_pricing.count();
            output.number_of_pricings++;
            if (alpha_cur == 0 && beta == 0)
                output.number_of_no_stab_pricings++;
            // Look for negative reduced cost columns.
            for (const Column& column: all_columns) {
                Value rc = compute_reduced_cost(column, duals_out);
                //std::cout << "rc " << rc << std::endl;
                if (parameters.objective_sense == ObjectiveSense::Min
                        && rc <= 0 - TOL)
                    new_columns.push_back(column);
                if (parameters.objective_sense == ObjectiveSense::Max
                        && rc >= 0 + TOL)
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

        // Get lagrangian constraint values.
        std::fill(lagrangian_constraint_values.begin(), lagrangian_constraint_values.end(), 0);
        for (const Column& column: all_columns) {
            for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
                RowIdx row_index = column.row_indices[row_pos];
                Value row_coefficient = column.row_coefficients[row_pos];
                lagrangian_constraint_values[row_index] += row_coefficient;
            }
        }
        // Compute subgradient at separation point.
        for (RowIdx row = 0; row < new_number_of_rows; ++row)
            subgradient[new_rows[row]]
                = std::min(0.0, new_row_upper_bounds[row] - lagrangian_constraint_values[new_rows[row]])
                + std::max(0.0, new_row_lower_bounds[row] - lagrangian_constraint_values[new_rows[row]]);

        // Adjust alpha.
        if (optional_parameters.self_adjusting_wentges_smoothing
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

        for (const Column& column: new_columns) {
            //std::cout << column << std::endl;
            // Add new column to the global column pool.
            parameters.columns.push_back(column);
            output.number_of_added_columns++;
            // Add new column to the local LP solver.
            std::vector<RowIdx> ri;
            std::vector<Value> rc;
            for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
                RowIdx i = column.row_indices[row_pos];
                Value c = column.row_coefficients[row_pos];
                if (new_row_indices[i] < 0)
                    continue;
                ri.push_back(new_row_indices[i]);
                rc.push_back(c);
            }
            solver_column_indices.push_back(parameters.columns.size() - 1);
            solver->add_column(
                    ri,
                    rc,
                    column.objective_coefficient,
                    parameters.column_lower_bound,
                    parameters.column_upper_bound);
        }
    }

    // Compute solution value.
    output.solution_value = c0 + solver->objective();

    // Compute solution
    for (ColIdx col = 0; col < (ColIdx)solver_column_indices.size(); ++col)
        if (solver_column_indices[col] != -1 && solver->primal(col) != 0)
            output.solution.push_back({
                    solver_column_indices[col],
                    solver->primal(col)});

    // Final display.
    double time = (double)std::round(optional_parameters.info.elapsed_time() * 10000) / 10000;
    VER(optional_parameters.info, std::defaultfloat
            << std::endl
            << "Final statistics" << std::endl
            << "----------------" << std::endl
            << "Solution:                         " << output.solution_value << std::endl
            << "Number of iterations:             " << output.number_of_iterations << std::endl
            << "Total number of columns:          " << parameters.columns.size() << std::endl
            << "Number of columns added:          " << output.number_of_added_columns << std::endl
            << "Number of pricing:                " << output.number_of_pricings << std::endl
            << "Number of 1st try pricing:        " << output.number_of_first_try_pricings << std::endl
            << "Number of mispricing:             " << output.number_of_mispricings << std::endl
            << "Number of pricing without stab.:  " << output.number_of_no_stab_pricings << std::endl
            << "Time LP solve (s):                " << output.time_lpsolve << std::endl
            << "Time pricing (s):                 " << output.time_pricing << std::endl
            << "Total time (s):                   " << time << std::endl);
    return output;
}

}
