/**
 * This file contains the implementation of the MILP solver.
 * 
 * It takes a GreedyOutput (TODO: add outputs to the other algorithms) and solves the MILP solution
 * of all generated columns. This may be useful when the various nodes have produced a combination
 * of columns that would result in a better primal solution than that found from by greedy by the
 * end node of the greedy algorithm.
 * 
 * For example, this may happen due to the dummy variable implementation in the column generation.
 * 
 */

#include "columngenerationsolver/algorithms/milp_solver.hpp"

using namespace columngenerationsolver;

// TODO: this is a temporary function to solve the integer problem, should be added to another file
// for better code organization once proven to be working.
// TODO: currently just returning the objective function, we should have a full solution with values for each variable.
void MilpSolverCbc::set_solver()
{
    // Set upper and lower bounds of all rows
    RowIdx number_of_rows = output_.solution.model().rows.size();
    std::vector<Value> row_lower_bounds(number_of_rows);
    std::vector<Value> row_upper_bounds(number_of_rows);
    // for (const Row& row: output_.model.rows) {
    for (RowIdx row_id = 0; row_id < number_of_rows; ++row_id) {
        row_lower_bounds[row_id] = output_.solution.model().rows[row_id].lower_bound;
        row_upper_bounds[row_id] = output_.solution.model().rows[row_id].upper_bound;
    }

    //get inspired by column_generation.cpp l295ish here to add the column information and then do 
    std::vector<Value> lower_bounds;
    std::vector<Value> upper_bounds;
    std::vector<Value> objective_coefficients;
    std::vector<std::vector<RowIdx>> row_ids;
    std::vector<std::vector<Value>> row_coefficients;
    for (const std::shared_ptr<const Column>& column_ptr: output_.columns) {
        std::vector<RowIdx> ri;
        std::vector<Value> rc;
        for (const LinearTerm& element: column_ptr->elements) {
            ri.push_back(element.row);
            rc.push_back(element.coefficient);
        }
        row_ids.push_back(ri);
        row_coefficients.push_back(rc);
        lower_bounds.push_back(column_ptr->lower_bound);
        upper_bounds.push_back(column_ptr->upper_bound);
        objective_coefficients.push_back(column_ptr->objective_coefficient);
    }
    
    // Create the LP relaxation solver
    LinearProgrammingSolverClp lp_solver = LinearProgrammingSolverClp(
                output_.solution.model().objective_sense,
                row_lower_bounds,
                row_upper_bounds);
    lp_solver.add_columns(
        row_ids,
        row_coefficients,
        objective_coefficients,
        lower_bounds,
        upper_bounds);

    ClpSimplex lp_model = lp_solver.get_model();
    const CoinPackedMatrix* matrix = lp_model.matrix();

    //TODO: everythin but the matrix was generated above so can simplify this.
    solver_.loadProblem(*matrix,
                    lp_model.getColLower(),
                    lp_model.getColUpper(),
                    lp_model.getObjCoefficients(),
                    lp_model.getRowLower(),
                    lp_model.getRowUpper());

    solver_.setObjSense(lp_model.getObjSense());
    //ClpModel lp_model_asclp = lp_model; // implicit upcasr
    //solver_.loadFromClpSimplex(lp_model);
    for (int i = 0; i < solver_.getNumCols(); ++i) {
        solver_.setInteger(i);
    }
}

void MilpSolverCbc::solve()
{
    model_.branchAndBound();
    if (model_.isProvenInfeasible()) {
        throw std::runtime_error("Infeasible model");
    }

    // Update the output with the solution, currently reinitializing. Can optimize later.
    SolutionBuilder milp_solution_builder;
    milp_solution_builder.set_model(output_.solution.model());
    for (int i = 0; i < model_.getNumCols(); ++i) {
        milp_solution_builder.add_column(output_.columns[i], primal(i));
    }
    Solution milp_solution = milp_solution_builder.build();
    output_.solution = milp_solution;
}

void MilpSolverCbc::print_solution()
{
    std::cout << "Number of feasible solutions: " << nb_feasible_solutions() << std::endl;
    std::cout << "Objective value: " << objective() << std::endl;
    for (int i = 0; i < model_.getNumCols(); ++i) {
        std::cout << "Column " << i << " value: " << primal(i) << std::endl;
    }
}