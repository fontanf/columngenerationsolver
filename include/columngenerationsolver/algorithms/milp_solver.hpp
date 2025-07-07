#pragma once

#include "columngenerationsolver/commons.hpp"
#include "columngenerationsolver/algorithms/greedy.hpp"
#include "linear_programming_solver.hpp"

#if CBC_FOUND
// Using Clp as the underlying solver.
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
        // Takes a fully formed linear program that could be solved by clp but we load it into cbc
        // so that we can change some values to integers and solve the full integer program.
        // Note this makes a copy of the model, alternative method to link to existing model - 
        // may not be as safe so doing it this way for now.
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
