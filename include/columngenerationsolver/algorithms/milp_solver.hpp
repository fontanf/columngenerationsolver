/**
 * Implementation of the MILP solver.
 * 
 * It takes a GreedyOutput and solves the MILP solution of all generated
 * columns. This may be useful when the various nodes of the greedy algorithm
 * have produced a combination of columns that would result in a better primal
 * solution than that found by the final node of the greedy algorithm.
 * 
 * For example, this may happen due to the dummy variable implementation in the
 * column generation.
 * 
 */

#pragma once

#include "columngenerationsolver/commons.hpp"
#include "columngenerationsolver/algorithms/greedy.hpp"
#include "linear_programming_solver.hpp"

#if CBC_FOUND
// Using Cbc as the underlying solver for milp problems.
#include <coin/CbcModel.hpp>
#include <coin/OsiClpSolverInterface.hpp>
#endif

namespace columngenerationsolver
{

class MilpSolver
{
public:
    virtual ~MilpSolver() { }
    virtual void solve() = 0;
    virtual Value objective() const = 0;
    virtual Value dual(RowIdx row) const = 0;
    virtual Value primal(ColIdx col) const = 0;
    virtual int nb_feasible_solutions() const = 0;
};

#if CBC_FOUND

class MilpSolverCbc: public MilpSolver
{

public:

    MilpSolverCbc(GreedyOutput& output)
        : output_(output)
    {
        set_solver();
        model_ = CbcModel(solver_);
        model_.messageHandler()->setLogLevel(0);
    }

    virtual ~MilpSolverCbc() { }


    void solve();
    Value objective() const { return model_.getObjValue(); }
    Value dual(RowIdx row) const { return model_.getRowPrice()[row]; }
    Value primal(ColIdx col) const { return model_.getCbcColSolution()[col]; }
    int nb_feasible_solutions() const { return model_.getSolutionCount(); }
    void print_solution();
    const GreedyOutput& output() const { return output_; }

private:

    CbcModel model_;
    OsiClpSolverInterface solver_;
    GreedyOutput output_;
    void set_solver();

};

#endif

}
