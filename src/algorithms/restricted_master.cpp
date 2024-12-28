#include "columngenerationsolver/algorithms/restricted_master.hpp"

#include "columngenerationsolver/algorithm_formatter.hpp"

#if CBC_FOUND
#include <coin/CbcModel.hpp>
#include <coin/CbcOrClpParam.hpp>
#include <coin/OsiCbcSolverInterface.hpp>
#endif

using namespace columngenerationsolver;

const RestrictedMasterOutput columngenerationsolver::restricted_master(
        const Model& model,
        const RestrictedMasterParameters& parameters)
{
    // Initial display.
    RestrictedMasterOutput output(model);
    AlgorithmFormatter algorithm_formatter(
            model,
            parameters,
            output);
    algorithm_formatter.start("Restricted master");
    algorithm_formatter.print_column_generation_header();

#if CBC_FOUND
    if (parameters.solver_name == MilpSolverName::CBC) {
        OsiCbcSolverInterface cbc_solver;
        std::vector<std::shared_ptr<const Column>> solver_columns;

        // Reduce printout.
        cbc_solver.getModelPtr()->setLogLevel(0);
        cbc_solver.messageHandler()->setLogLevel(2);

        // Load problem.
        if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize) {
            cbc_solver.setObjSense(1);
        } else {
            cbc_solver.setObjSense(-1);
        }

        // Add constraints.
        for (RowIdx row_id = 0;
                row_id < (RowIdx)model.rows.size();
                ++row_id) {
            Value lb = model.rows[row_id].lower_bound;
            Value ub = model.rows[row_id].upper_bound;
            cbc_solver.addRow(
                    CoinPackedVector(),
                    (lb != -std::numeric_limits<Value>::infinity())? lb: -COIN_DBL_MAX,
                    (ub != +std::numeric_limits<Value>::infinity())? ub: COIN_DBL_MAX);
        }

        // Add model columns.
        std::cout << "model columns..." << std::endl;
        for (const std::shared_ptr<const Column>& column: model.columns) {
            std::vector<int> row_indices;
            std::vector<Value> row_coefficients;
            for (const LinearTerm& element: column->elements) {
                row_indices.push_back(element.row);
                row_coefficients.push_back(element.coefficient);
            }
            solver_columns.push_back(column);
            cbc_solver.addCol(
                    row_indices.size(),
                    row_indices.data(),
                    row_coefficients.data(),
                    ((column->lower_bound != -std::numeric_limits<Value>::infinity())? column->lower_bound: -COIN_DBL_MAX),
                    ((column->upper_bound != std::numeric_limits<Value>::infinity())? column->upper_bound: COIN_DBL_MAX),
                    column->objective_coefficient);
            if (column->type == VariableType::Integer)
                cbc_solver.setInteger(solver_columns.size() - 1);
        }
        // Add columns from column pool.
        std::cout << "column pool..." << std::endl;
        for (const std::shared_ptr<const Column>& column: parameters.column_pool) {
            std::vector<int> row_indices;
            std::vector<Value> row_coefficients;
            for (const LinearTerm& element: column->elements) {
                row_indices.push_back(element.row);
                row_coefficients.push_back(element.coefficient);
            }
            solver_columns.push_back(column);
            cbc_solver.addCol(
                    row_indices.size(),
                    row_indices.data(),
                    row_coefficients.data(),
                    ((column->lower_bound != -std::numeric_limits<Value>::infinity())? column->lower_bound: -COIN_DBL_MAX),
                    ((column->upper_bound != std::numeric_limits<Value>::infinity())? column->upper_bound: COIN_DBL_MAX),
                    column->objective_coefficient);
            if (column->type == VariableType::Integer)
                cbc_solver.setInteger(solver_columns.size() - 1);
        }

        // Pass data and solver to CbcModel.
        CbcModel cbc_model(cbc_solver);

        // Callback.
        //EventHandler event_handler(instance, parameters, output, algorithm_formatter);
        //cbc_model.passInEventHandler(&event_handler);

        // Reduce printout.
        cbc_model.setLogLevel(1);
        //cbc_model.solver()->setHintParam(OsiDoReducePrint, true, OsiHintTry);

        // Set time limit.
        cbc_model.setMaximumSeconds(parameters.timer.remaining_time());

        // Add initial solution.
        std::cout << "initial solution..." << std::endl;
        if (parameters.initial_solution != nullptr) {
            ColumnMap initial_solution;
            for (const auto& p: parameters.initial_solution->columns())
                initial_solution.set_column_value(p.first, p.second);

            std::vector<double> cbc_initial_solution(solver_columns.size());
            for (ColIdx column_id = 0;
                    column_id < (ColIdx)solver_columns.size();
                    ++column_id) {
                cbc_initial_solution[column_id] = initial_solution.get_column_value(
                        solver_columns[column_id],
                        0.0);
            }
            cbc_model.setBestSolution(
                    cbc_initial_solution.data(),
                    solver_columns.size(),
                    -parameters.initial_solution->objective_value());
        }

        // Set maximum number of nodes.
        if (parameters.maximum_number_of_nodes != -1)
            cbc_model.setMaximumNodes(parameters.maximum_number_of_nodes);

        // Do complete search.
        std::cout << "solve..." << std::endl;
        cbc_model.branchAndBound();

        std::cout << "retrieve solution..." << std::endl;
        if (cbc_model.isProvenInfeasible()) {
            // Infeasible.

        } else if (cbc_model.bestSolution() != NULL) {
            std::cout << "feasible..." << std::endl;
            // Feasible solution found.

            // Update primal solution.
            const double* cbc_solution = cbc_model.solver()->getColSolution();
            SolutionBuilder solution_builder;
            solution_builder.set_model(model);
            for (ColIdx column_id = 0;
                    column_id < (ColIdx)solver_columns.size();
                    ++column_id) {
                if (cbc_solution[column_id] > 0) {
                    solution_builder.add_column(
                            solver_columns[column_id],
                            cbc_solution[column_id]);
                }
            }
            Solution solution = solution_builder.build();
            algorithm_formatter.update_solution(
                    solution);
        }
    }
#endif

    std::cout << "end..." << std::endl;
    algorithm_formatter.end();
    return output;
}
