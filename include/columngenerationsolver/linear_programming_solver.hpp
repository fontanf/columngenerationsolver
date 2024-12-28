#pragma once

#include "columngenerationsolver/commons.hpp"

#if CLP_FOUND
#include <coin/ClpModel.hpp>
#include <coin/CbcOrClpParam.hpp>
#include <coin/OsiClpSolverInterface.hpp>
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

enum class SolverName { CLP, CPLEX, Xpress, Knitro };

inline std::istream& operator>>(
        std::istream& in,
        SolverName& solver_name)
{
    std::string token;
    in >> token;
    if (token == "clp" || token == "CLP") {
        solver_name = SolverName::CLP;
    } else if (token == "cplex" || token == "Cplex" || token == "CPLEX") {
        solver_name = SolverName::CPLEX;
    } else if (token == "xpress" || token == "Xpress" || token == "XPRESS") {
        solver_name = SolverName::Xpress;
    } else if (token == "knitro" || token == "Knitro") {
        solver_name = SolverName::Knitro;
    } else  {
        in.setstate(std::ios_base::failbit);
    }
    return in;
}

inline std::ostream& operator<<(
        std::ostream& os,
        SolverName solver_name)
{
    switch (solver_name) {
    case SolverName::CLP: {
        os << "CLP";
        break;
    } case SolverName::CPLEX: {
        os << "CPLEX";
        break;
    } case SolverName::Xpress: {
        os << "Xpress";
        break;
    } case SolverName::Knitro: {
        os << "Knitro";
        break;
    }
    }
    return os;
}

inline SolverName s2lps(const std::string& s)
{
    if (s == "clp" || s == "CLP") {
        return SolverName::CLP;
    } else if (s == "cplex" || s == "Cplex" || s == "CPLEX") {
        return SolverName::CPLEX;
    } else if (s == "xpress" || s == "Xpress" || s == "XPRESS") {
        return SolverName::Xpress;
    } else if (s == "knitro" || s == "Knitro") {
        return SolverName::Knitro;
    } else {
        return SolverName::CLP;
    }
}

class LinearProgrammingSolver
{
public:
    virtual ~LinearProgrammingSolver() { }
    virtual void add_column(
            const std::vector<RowIdx>& row_indices,
            const std::vector<Value>& row_coefficients,
            Value objective_coefficient,
            Value lower_bound,
            Value upper_bound) = 0;
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
        for (RowIdx row_id = 0;
                row_id < (RowIdx)row_lower_bounds.size();
                ++row_id) {
            model_.addRow(
                    0, NULL, NULL,
                    (row_lower_bounds[row_id] != -std::numeric_limits<Value>::infinity())? row_lower_bounds[row_id]: -COIN_DBL_MAX,
                    (row_upper_bounds[row_id] != std::numeric_limits<Value>::infinity())? row_upper_bounds[row_id]: COIN_DBL_MAX);
        }
    }

    virtual ~LinearProgrammingSolverClp() { }

    void add_column(
            const std::vector<RowIdx>& row_indices,
            const std::vector<Value>& row_coefficients,
            Value objective_coefficient,
            Value lower_bound,
            Value upper_bound)
    {
        std::vector<int> row_indices_int(row_indices.begin(), row_indices.end());
        model_.addColumn(
                row_indices.size(),
                row_indices_int.data(),
                row_coefficients.data(),
                ((lower_bound != -std::numeric_limits<Value>::infinity())? lower_bound: -COIN_DBL_MAX),
                ((upper_bound != std::numeric_limits<Value>::infinity())? upper_bound: COIN_DBL_MAX),
                objective_coefficient);
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

    void add_column(
            const std::vector<RowIdx>& row_indices,
            const std::vector<Value>& row_coefficients,
            Value objective_coefficient,
            Value lower_bound,
            Value upper_bound)
    {
        //std::cout << "LinearProgrammingSolverXpress::add_column" << std::endl;
        primals_.push_back(0.0);
        basis_cols_.push_back(0);
        int start[] = {0};
        for (int i = 0; i < (int)row_indices.size(); ++i)
            ri_[i] = row_indices[i];
        XPRSaddcols(
                problem_,
                1,
                row_indices.size(),
                &objective_coefficient,
                start,
                ri_.data(),
                row_coefficients.data(),
                &lower_bound,
                &upper_bound);
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
