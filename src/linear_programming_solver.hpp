#pragma once

#include "columngenerationsolver/commons.hpp"

#if CLP_FOUND
#include <coin/ClpSimplex.hpp>
#endif

#if HIGHS_FOUND
#include <Highs.h>
#endif

#if CPLEX_FOUND
#include <ilcplex/ilocplex.h>
#endif

#if XPRESS_FOUND
#include <xprs.h>
#endif

#if KNITRO_FOUND
#include <knitro.h>
#endif

namespace columngenerationsolver
{

class LinearProgrammingSolver
{
public:
    virtual ~LinearProgrammingSolver() { }
    virtual void add_column(
            const std::vector<RowIdx>& row_indices,
            const std::vector<Value>& row_coefficients,
            Value objective_coefficient,
            Value lower_bound,
            Value upper_bound)
    {
        add_columns(
                {row_indices},
                {row_coefficients},
                {objective_coefficient},
                {lower_bound},
                {upper_bound});
    }
    virtual void add_columns(
            const std::vector<std::vector<RowIdx>>& row_indices,
            const std::vector<std::vector<Value>>& row_coefficients,
            const std::vector<Value>& objective_coefficient,
            const std::vector<Value>& lower_bound,
            const std::vector<Value>& upper_bound) = 0;
    virtual void solve() = 0;
    virtual Value objective() const = 0;
    virtual Value dual(RowIdx row) const = 0;
    virtual Value primal(ColIdx col) const = 0;
};

#if CLP_FOUND

class LinearProgrammingSolverClp: public LinearProgrammingSolver
{

public:

    LinearProgrammingSolverClp(
            optimizationtools::ObjectiveDirection objective_sense,
            const std::vector<Value>& row_lower_bounds,
            const std::vector<Value>& row_upper_bounds)
    {
        model_.messageHandler()->setLogLevel(0);
        //model_.setLogLevel(99);
        if (objective_sense == optimizationtools::ObjectiveDirection::Minimize) {
            model_.setOptimizationDirection(1);
        } else {
            model_.setOptimizationDirection(-1);
        }
        std::vector<double> clp_lower_bounds(row_lower_bounds.size());
        std::vector<double> clp_upper_bounds(row_upper_bounds.size());
        for (RowIdx row_id = 0;
                row_id < (RowIdx)row_lower_bounds.size();
                ++row_id) {
            clp_lower_bounds[row_id] = (row_lower_bounds[row_id] != -std::numeric_limits<Value>::infinity())? row_lower_bounds[row_id]: -COIN_DBL_MAX;
            clp_upper_bounds[row_id] = (row_upper_bounds[row_id] != +std::numeric_limits<Value>::infinity())? row_upper_bounds[row_id]: +COIN_DBL_MAX;
        }
        model_.addRows(
                row_lower_bounds.size(),
                clp_lower_bounds.data(),
                clp_upper_bounds.data(),
                NULL, NULL, NULL);
    }

    virtual ~LinearProgrammingSolverClp() { }

    void add_columns(
            const std::vector<std::vector<RowIdx>>& row_indices,
            const std::vector<std::vector<Value>>& row_coefficients,
            const std::vector<Value>& objective_coefficients,
            const std::vector<Value>& lower_bounds,
            const std::vector<Value>& upper_bounds)
    {
        std::vector<double> clp_lower_bounds(lower_bounds.size());
        std::vector<double> clp_upper_bounds(upper_bounds.size());
        std::vector<double> clp_objective(objective_coefficients.size());
        std::vector<CoinBigIndex> clp_column_starts(row_indices.size() + 1);
        RowIdx number_of_elements = 0;
        for (const auto& e: row_coefficients)
            number_of_elements += e.size();
        std::vector<int> clp_column_rows(number_of_elements);
        std::vector<double> clp_column_elements(number_of_elements);

        RowIdx pos = 0;
        for (ColIdx col = 0; col < (ColIdx)row_indices.size(); ++col) {
            clp_lower_bounds[col] = ((lower_bounds[col] != -std::numeric_limits<Value>::infinity())? lower_bounds[col]: -COIN_DBL_MAX);
            clp_upper_bounds[col] = ((upper_bounds[col] != +std::numeric_limits<Value>::infinity())? upper_bounds[col]: +COIN_DBL_MAX);
            clp_objective[col] = objective_coefficients[col];
            clp_column_starts[col] = pos;
            for (RowIdx row = 0; row < (RowIdx)row_indices[col].size(); ++row) {
                clp_column_rows[pos] = row_indices[col][row];
                clp_column_elements[pos] = row_coefficients[col][row];
                pos++;
            }
        }
        clp_column_starts[row_indices.size()] = pos;

        model_.addColumns(
                row_indices.size(),
                clp_lower_bounds.data(),
                clp_upper_bounds.data(),
                clp_objective.data(),
                clp_column_starts.data(),
                clp_column_rows.data(),
                clp_column_elements.data());
    }

    void solve()
    {
        //model_.writeLp("output");
        model_.primal();
        if (model_.isProvenPrimalInfeasible()) {
            model_.writeLp("output");
            throw std::runtime_error("Infeasible model");
        }
    }

    Value objective() const { return model_.objectiveValue(); }
    Value dual(RowIdx row) const { return model_.dualRowSolution()[row]; }
    Value primal(ColIdx col) const { return model_.getColSolution()[col]; }

private:

    ClpSimplex model_;

};

#endif

#if HIGHS_FOUND

class LinearProgrammingSolverHighs: public LinearProgrammingSolver
{

public:

    LinearProgrammingSolverHighs(
            optimizationtools::ObjectiveDirection objective_sense,
            const std::vector<Value>& row_lower_bounds,
            const std::vector<Value>& row_upper_bounds)
    {
        // Reduce printout.
        model_.setOptionValue("log_to_console", false);
        model_.setOptionValue("simplex_strategy", 4);

        if (objective_sense == optimizationtools::ObjectiveDirection::Minimize) {
            model_.changeObjectiveSense(ObjSense::kMinimize);
        } else {
            model_.changeObjectiveSense(ObjSense::kMaximize);
        }
        std::vector<double> highs_lower_bounds(row_lower_bounds.size());
        std::vector<double> highs_upper_bounds(row_upper_bounds.size());
        for (RowIdx row_id = 0;
                row_id < (RowIdx)row_lower_bounds.size();
                ++row_id) {
            highs_lower_bounds[row_id] = (row_lower_bounds[row_id] != -std::numeric_limits<Value>::infinity())? row_lower_bounds[row_id]: -1.0e30;
            highs_upper_bounds[row_id] = (row_upper_bounds[row_id] != +std::numeric_limits<Value>::infinity())? row_upper_bounds[row_id]: +1.0e30;
        }
        model_.addRows(
                row_lower_bounds.size(),
                highs_lower_bounds.data(),
                highs_upper_bounds.data(),
                0, NULL, NULL, NULL);
    }

    virtual ~LinearProgrammingSolverHighs() { }

    void add_columns(
            const std::vector<std::vector<RowIdx>>& row_indices,
            const std::vector<std::vector<Value>>& row_coefficients,
            const std::vector<Value>& objective_coefficients,
            const std::vector<Value>& lower_bounds,
            const std::vector<Value>& upper_bounds)
    {
        std::vector<double> highs_lower_bounds(lower_bounds.size());
        std::vector<double> highs_upper_bounds(upper_bounds.size());
        std::vector<double> highs_objective(objective_coefficients.size());
        std::vector<HighsInt> highs_column_starts(row_indices.size() + 1);
        RowIdx number_of_elements = 0;
        for (const auto& e: row_coefficients)
            number_of_elements += e.size();
        std::vector<int> highs_column_rows(number_of_elements);
        std::vector<double> highs_column_elements(number_of_elements);

        RowIdx pos = 0;
        for (ColIdx col = 0; col < (ColIdx)row_indices.size(); ++col) {
            highs_lower_bounds[col] = ((lower_bounds[col] != -std::numeric_limits<Value>::infinity())? lower_bounds[col]: -1.0e30);
            highs_upper_bounds[col] = ((upper_bounds[col] != +std::numeric_limits<Value>::infinity())? upper_bounds[col]: +1.0e30);
            highs_objective[col] = objective_coefficients[col];
            highs_column_starts[col] = pos;
            for (RowIdx row = 0; row < (RowIdx)row_indices[col].size(); ++row) {
                highs_column_rows[pos] = row_indices[col][row];
                highs_column_elements[pos] = row_coefficients[col][row];
                pos++;
            }
        }
        highs_column_starts[row_indices.size()] = pos;

        model_.addCols(
                row_indices.size(),
                highs_objective.data(),
                highs_lower_bounds.data(),
                highs_upper_bounds.data(),
                number_of_elements,
                highs_column_starts.data(),
                highs_column_rows.data(),
                highs_column_elements.data());
    }

    void solve()
    {
        //model_.writeLp("output");
        model_.run();
    }

    Value objective() const { return model_.getObjectiveValue(); }
    Value dual(RowIdx row) const { return model_.getSolution().row_dual[row]; }
    Value primal(ColIdx col) const { return model_.getSolution().col_value[col]; }

private:

    Highs model_;

};

#endif

#if CPLEX_FOUND

ILOSTLBEGIN

class LinearProgrammingSolverCplex: public LinearProgrammingSolver
{

public:

    LinearProgrammingSolverCplex(
            optimizationtools::ObjectiveDirection objective_sense,
            const std::vector<Value>& row_lower_bounds,
            const std::vector<Value>& row_upper_bounds):
        env_(),
        model_(env_),
        obj_(env_),
        ranges_(env_),
        cplex_(model_)
    {
        if (objective_sense == optimizationtools::ObjectiveDirection::Minimize) {
            obj_.setSense(IloObjective::Minimize);
        } else {
            obj_.setSense(IloObjective::Maximize);
        }
        model_.add(obj_);
        for (RowIdx i = 0; i < (RowIdx)row_lower_bounds.size(); ++i)
            ranges_.add(IloRange(
                        env_,
                        (row_lower_bounds[i] != -std::numeric_limits<Value>::infinity())? row_lower_bounds[i]: -IloInfinity,
                        (row_upper_bounds[i] !=  std::numeric_limits<Value>::infinity())? row_upper_bounds[i]: IloInfinity));
        model_.add(ranges_);
        cplex_.setOut(env_.getNullStream()); // Remove standard output
        //cplex_.setParam(IloCplex::Param::Threads, 1);
        //cplex_.setParam(IloCplex::Param::Preprocessing::Presolve, 0);
    }

    virtual ~LinearProgrammingSolverCplex()
    {
        env_.end();
    }

    void add_column(
            const std::vector<RowIdx>& row_indices,
            const std::vector<Value>& row_coefficients,
            Value objective_coefficient,
            Value lower_bound,
            Value upper_bound)
    {
        IloNumColumn col = obj_(objective_coefficient);
        for (RowIdx i = 0; i < (RowIdx)row_indices.size(); ++i)
            col += ranges_[row_indices[i]](row_coefficients[i]);
        vars_.push_back(IloNumVar(
                    col,
                    ((lower_bound != -std::numeric_limits<Value>::infinity())? lower_bound: -IloInfinity),
                    ((upper_bound != std::numeric_limits<Value>::infinity())? upper_bound: IloInfinity),
                    ILOFLOAT));
        model_.add(vars_.back());
    }

    void solve()
    {
        //std::cout << model_ << std::endl;
        cplex_.solve();
    }

    Value objective() const { return cplex_.getObjValue(); }
    Value dual(RowIdx row) const { return cplex_.getDual(ranges_[row]); }
    Value primal(ColIdx col) const { return cplex_.getValue(vars_[col]); }

private:

    IloEnv env_;
    IloModel model_;
    IloObjective obj_;
    IloRangeArray ranges_;
    std::vector<IloNumVar> vars_;
    IloCplex cplex_;

};

#endif

#if XPRESS_FOUND

class LinearProgrammingSolverXpress: public LinearProgrammingSolver
{

public:

    LinearProgrammingSolverXpress(
            optimizationtools::ObjectiveDirection objective_sense,
            const std::vector<Value>& row_lower_bounds,
            const std::vector<Value>& row_upper_bounds)
    {
        //std::cout << "LinearProgrammingSolverXpress::LinearProgrammingSolverXpress" << std::endl;
        XPRScreateprob(&problem_);
        XPRSsetintcontrol(problem_, XPRS_THREADS, 1);
        //XPRSsetlogfile(problem_, "xpress.log");
        // Objective.
        if (objective_sense == optimizationtools::ObjectiveDirection::Minimize) {
            XPRSchgobjsense(problem_, XPRS_OBJ_MINIMIZE);
        } else {
            XPRSchgobjsense(problem_, XPRS_OBJ_MAXIMIZE);
        }
        // Constraints.
        duals_ = std::vector<double>(row_lower_bounds.size(), 0.0);
        basis_rows_ = std::vector<int>(row_lower_bounds.size(), 0.0);
        ri_ = std::vector<int>(row_lower_bounds.size());
        std::vector<double> rhs(row_lower_bounds.size(), 0.0);
        std::vector<double> rng(row_lower_bounds.size(), 0.0);
        std::vector<char> row_types(row_lower_bounds.size());
        for (RowIdx i = 0; i < (RowIdx)row_lower_bounds.size(); ++i) {
            rhs[i] = row_upper_bounds[i];
            rng[i] = row_upper_bounds[i] - row_lower_bounds[i];
            row_types[i] = 'R';
            if (row_lower_bounds[i] == row_upper_bounds[i])
                row_types[i] = 'E';
        }
        XPRSaddrows(
                problem_,
                row_lower_bounds.size(),
                0,
                row_types.data(),
                rhs.data(),
                rng.data(),
                NULL,
                NULL,
                NULL);
    }

    virtual ~LinearProgrammingSolverXpress()
    {
        XPRSdestroyprob(problem_);
    }

    void add_columns(
            const std::vector<std::vector<RowIdx>>& row_indices,
            const std::vector<std::vector<Value>>& row_coefficients,
            const std::vector<Value>& objective_coefficients,
            const std::vector<Value>& lower_bounds,
            const std::vector<Value>& upper_bounds)
    {
        std::vector<double> xprs_objective(objective_coefficients.size());
        std::vector<int> xprs_column_starts(row_indices.size() + 1);
        RowIdx number_of_elements = 0;
        for (const auto& e: row_coefficients)
            number_of_elements += e.size();
        std::vector<int> xprs_column_rows(number_of_elements);
        std::vector<double> xprs_column_elements(number_of_elements);
        std::vector<int> xprs_row_indices(number_of_elements);

        RowIdx pos = 0;
        for (ColIdx col = 0; col < (ColIdx)row_indices.size(); ++col) {
            primals_.push_back(0.0);
            basis_cols_.push_back(0);
            xprs_objective[col] = objective_coefficients[col];
            xprs_column_starts[col] = pos;
            for (RowIdx row = 0; row < (RowIdx)row_indices[col].size(); ++row) {
                xprs_column_rows[pos] = row_indices[col][row];
                xprs_column_elements[pos] = row_coefficients[col][row];
                xprs_row_indices[pos] = row_indices[col][row];
                pos++;
            }
        }
        xprs_column_starts[row_indices.size()] = pos;

        XPRSaddcols(
                problem_,
                row_indices.size(),
                xprs_column_elements.size(),
                objective_coefficients.data(),
                xprs_column_starts.data(),
                xprs_row_indices.data(),
                xprs_column_elements.data(),
                lower_bounds.data(),
                upper_bounds.data());
    }

    void solve()
    {
        //std::cout << "LinearProgrammingSolverXpress::solve" << std::endl;
        if (primals_.empty())
            return;
        if (has_basis_)
            XPRSloadbasis(problem_, basis_rows_.data(), basis_cols_.data());
        XPRSlpoptimize(problem_, "");
        XPRSgetlpsol(problem_, primals_.data(), NULL, duals_.data(), NULL);
        XPRSgetbasis(problem_, basis_rows_.data(), basis_cols_.data());
        has_basis_ = true;
    }

    Value objective() const
    {
        if (primals_.empty())
            return 0.0;
        double objective_value = 0.0;
        XPRSgetdblattrib(problem_, XPRS_LPOBJVAL, &objective_value);
        return objective_value;
    }

    Value dual(RowIdx row) const
    {
        return duals_[row];
    }

    Value primal(ColIdx col) const { return primals_[col]; }

private:

    XPRSprob problem_;
    bool has_basis_ = false;
    std::vector<int> ri_;
    std::vector<int> basis_rows_;
    std::vector<int> basis_cols_;
    std::vector<double> primals_;
    std::vector<double> duals_;

};

#endif

#if KNITRO_FOUND

class LinearProgrammingSolverKnitro: public LinearProgrammingSolver
{

public:

    LinearProgrammingSolverKnitro(
            optimizationtools::ObjectiveDirection objective_sense,
            const std::vector<Value>& row_lower_bounds,
            const std::vector<Value>& row_upper_bounds)
    {
        KN_new(&kc_);
        KN_set_param_by_name(kc_, "outlev", 4);
        if (objective_sense == optimizationtools::ObjectiveDirection::Minimize) {
            KN_set_obj_goal(kc_, KN_OBJGOAL_MINIMIZE);
        } else {
            KN_set_obj_goal(kc_, KN_OBJGOAL_MAXIMIZE);
        }
        KN_add_cons(kc_, row_lower_bounds.size(), NULL);
        for (RowIdx i = 0; i < (RowIdx)row_lower_bounds.size(); ++i) {
            if (row_lower_bounds[i] != -std::numeric_limits<Value>::infinity())
                KN_set_con_lobnd(kc_, i, row_lower_bounds[i]);
            if (row_upper_bounds[i] !=  std::numeric_limits<Value>::infinity())
                KN_set_con_upbnd(kc_, i, row_upper_bounds[i]);
        }
    }

    virtual ~LinearProgrammingSolverKnitro()
    {
        KN_free(&kc_);
    }

    void add_column(
            const std::vector<RowIdx>& row_indices,
            const std::vector<Value>& row_coefficients,
            Value objective_coefficient,
            Value lower_bound,
            Value upper_bound)
    {
        KNINT idx = -1;
        KN_add_var(kc_, &idx);
        if (lower_bound != -std::numeric_limits<Value>::infinity())
            KN_set_var_lobnd(kc_, idx, lower_bound);
        if (upper_bound != std::numeric_limits<Value>::infinity())
            KN_set_var_upbnd(kc_, idx, upper_bound);
        KN_add_obj_linear_struct(kc_, 1, &idx, &objective_coefficient);

        std::vector<KNINT> row_indices_kn;
        for (auto row: row_indices)
            row_indices_kn.push_back(row);
        std::vector<KNINT> col_indices_kn(row_indices.size(), idx);

        KN_add_con_linear_struct(
                kc_,
                row_indices.size(),
                row_indices_kn.data(),
                col_indices_kn.data(),
                row_coefficients.data());
    }

    void solve()
    {
        KN_solve(kc_);
    }

    Value objective() const
    {
        double obj = 0;
        KN_get_obj_value(kc_, &obj);
        return obj;
    }
    Value dual(RowIdx row) const
    {
        double d = 0;
        KN_get_con_dual_value(kc_, row, &d);
        return -d;
    }
    Value primal(ColIdx col) const
    {
        double p = 0;
        KN_get_var_primal_value(kc_, col, &p);
        return p;
    }

private:

    KN_context* kc_;

};

#endif

}
