#pragma once

#include "optimizationtools/info.hpp"

#include <vector>
#include <cstdint>
#include <limits>
#include <set>

#if CPLEX_FOUND
#include <ilcplex/ilocplex.h>
#endif

#if COINOR_FOUND
#include <coin/ClpModel.hpp>
#include <coin/OsiClpSolverInterface.hpp>
#endif

namespace columngenerationsolver
{

typedef int64_t Counter;
typedef int64_t ColIdx;
typedef int64_t RowIdx;
typedef double  Value;

enum class LinearProgrammingSolver { CLP, CPLEX, Xpress };
enum class ObjectiveSense { Min, Max };
inline std::istream& operator>>(std::istream& in, LinearProgrammingSolver& linear_programming_solver);
inline std::ostream& operator<<(std::ostream &os, LinearProgrammingSolver linear_programming_solver);

struct Column
{
    std::vector<RowIdx> row_indices;
    std::vector<Value> row_coefficients;
    Value objective_coefficient = 0;

    Value branching_priority = 0;
    void* extra;
};

inline std::ostream& operator<<(std::ostream &os, const Column& column);

class PricingSolver
{
public:
    virtual ~PricingSolver() { }
    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns) = 0;
    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals) = 0;
};

struct Parameters
{
    Parameters(RowIdx row_number):
        row_lower_bounds(row_number),
        row_upper_bounds(row_number),
        row_coefficient_lower_bounds(row_number),
        row_coefficient_upper_bounds(row_number) { }
    ObjectiveSense objective_sense = ObjectiveSense::Min;
    Value column_lower_bound;
    Value column_upper_bound;
    std::vector<Value> row_lower_bounds;
    std::vector<Value> row_upper_bounds;
    std::vector<Value> row_coefficient_lower_bounds;
    std::vector<Value> row_coefficient_upper_bounds;
    Value dummy_column_objective_coefficient;
    LinearProgrammingSolver linear_programming_solver = LinearProgrammingSolver::CLP;
    std::unique_ptr<PricingSolver> pricing_solver = NULL;
    std::vector<Column> columns;
};

/***************************** Column Generation ******************************/

struct ColumnGenerationOptionalParameters
{
    const std::vector<std::pair<ColIdx, Value>>* fixed_columns = NULL;

    Counter iteration_limit = -1;
    bool* end = NULL;

    // Stabilization parameters.
    Value static_wentges_smoothing_parameter = 0.5; // alpha
    bool self_adjusting_wentges_smoothing = true;
    Value static_directional_smoothing_parameter = 0.0; // beta
    bool automatic_directional_smoothing = true;

    optimizationtools::Info info = optimizationtools::Info();
};

struct ColumnGenerationOutput
{
    std::vector<std::pair<ColIdx, Value>> solution;
    Value solution_value;
    Counter iteration_number = 0;
    Counter added_column_number = 0;
    Counter goodpricing_number = 0;
    Counter pricing_number = 0;
    Counter first_try_pricing_number = 0;
    Counter mispricing_number = 0;
    Counter no_stab_pricing_number = 0;
    double time_pricing = 0.0;
    double time_lpsolve = 0.0;
};

inline ColumnGenerationOutput columngeneration(
        Parameters& parameters,
        ColumnGenerationOptionalParameters optional_parameters = {});

/*********************************** Greedy ***********************************/

struct GreedyOptionalParameters
{
    bool* end = NULL;
    ColumnGenerationOptionalParameters columngeneration_parameters;
    optimizationtools::Info info = optimizationtools::Info();
};

struct GreedyOutput
{
    std::vector<std::pair<ColIdx, Value>> solution;
    Value solution_value = 0;
    Value bound;
    double time_lpsolve = 0.0;
    double time_pricing = 0.0;
    Counter total_column_number = 0;
    Counter added_column_number = 0;
};

inline GreedyOutput greedy(
        Parameters& parameters,
        GreedyOptionalParameters optional_parameters = {});

/************************* Limited Discrepancy Search *************************/

struct LimitedDiscrepancySearchOutput
{
    std::vector<std::pair<ColIdx, Value>> solution;
    Value solution_value = 0;
    Value solution_discrepancy = -1;
    Value bound;
    Counter node_number = 0;
    double time_lpsolve = 0.0;
    double time_pricing = 0.0;
    Counter total_column_number = 0;
    Counter added_column_number = 0;
};

typedef std::function<void(const LimitedDiscrepancySearchOutput&)> LimitedDiscrepancySearchCallback;

struct LimitedDiscrepancySearchOptionalParameters
{
    LimitedDiscrepancySearchCallback new_bound_callback
        = [](const LimitedDiscrepancySearchOutput& o) { (void)o; };
    Counter thread_number = 3;
    Value discrepancy_limit = std::numeric_limits<Value>::infinity();
    bool* end = NULL;
    ColumnGenerationOptionalParameters columngeneration_parameters;
    optimizationtools::Info info = optimizationtools::Info();
};

inline LimitedDiscrepancySearchOutput limiteddiscrepancysearch(
        Parameters& parameters,
        LimitedDiscrepancySearchOptionalParameters = {});

/*************************** Heuristic Tree Search ****************************/

struct HeuristicTreeSearchOutput
{
    std::vector<std::pair<ColIdx, Value>> solution;
    Value solution_value = 0;
    Value bound;
    Counter total_column_number = 0;
    Counter added_column_number = 0;
};

typedef std::function<void(const HeuristicTreeSearchOutput&)> HeuristicTreeSearchCallback;

struct HeuristicTreeSearchOptionalParameters
{
    HeuristicTreeSearchCallback new_bound_callback
        = [](const HeuristicTreeSearchOutput& o) { (void)o; };
    Counter thread_number = 3;
    double growth_rate = 1.5;
    bool* end = NULL;
    ColumnGenerationOptionalParameters columngeneration_parameters;
    optimizationtools::Info info = optimizationtools::Info();
};

inline HeuristicTreeSearchOutput heuristictreesearch(
        Parameters& parameters,
        HeuristicTreeSearchOptionalParameters = {});

/******************************************************************************/

class ColumnGenerationSolver
{
public:
    virtual ~ColumnGenerationSolver() { }
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

#if COINOR_FOUND

class ColumnGenerationSolverClp: public ColumnGenerationSolver
{

public:

    ColumnGenerationSolverClp(
            ObjectiveSense objective_sense,
            const std::vector<Value>& row_lower_bounds,
            const std::vector<Value>& row_upper_bounds)
    {
        model_.messageHandler()->setLogLevel(0);
        if (objective_sense == ObjectiveSense::Min) {
            model_.setOptimizationDirection(1);
        } else {
            model_.setOptimizationDirection(-1);
        }
        for (RowIdx i = 0; i < (RowIdx)row_lower_bounds.size(); ++i)
            model_.addRow(
                    0, NULL, NULL,
                    (row_lower_bounds[i] != -std::numeric_limits<Value>::infinity())? row_lower_bounds[i]: -COIN_DBL_MAX,
                    (row_upper_bounds[i] !=  std::numeric_limits<Value>::infinity())? row_upper_bounds[i]: COIN_DBL_MAX);
    }

    virtual ~ColumnGenerationSolverClp() { }

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
                lower_bound,
                upper_bound,
                objective_coefficient);
    }

    void solve()
    {
        //model_.writeLp("output");
        model_.primal();
    }

    Value objective()        const { return model_.objectiveValue(); }
    Value dual(RowIdx row)   const { return model_.dualRowSolution()[row]; }
    Value primal(ColIdx col) const { return model_.getColSolution()[col]; }

private:

    ClpSimplex model_;

};

#endif

#if CPLEX_FOUND

ILOSTLBEGIN

class ColumnGenerationSolverCplex: public ColumnGenerationSolver
{

public:

    ColumnGenerationSolverCplex(
            ObjectiveSense objective_sense,
            const std::vector<Value>& row_lower_bounds,
            const std::vector<Value>& row_upper_bounds):
        env_(),
        model_(env_),
        obj_(env_),
        ranges_(env_),
        cplex_(model_)
    {
        if (objective_sense == ObjectiveSense::Min) {
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

    virtual ~ColumnGenerationSolverCplex()
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
        vars_.push_back(IloNumVar(col, lower_bound, upper_bound, ILOFLOAT));
        model_.add(vars_.back());
    }

    void solve()
    {
        //std::cout << model_ << std::endl;
        cplex_.solve();
    }

    Value objective()        const { return cplex_.getObjValue(); }
    Value dual(RowIdx row)   const { return cplex_.getDual(ranges_[row]); }
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

/******************************* Implementation *******************************/

std::istream& operator>>(std::istream& in, LinearProgrammingSolver& linear_programming_solver)
{
    std::string token;
    in >> token;
    if (token == "clp" || token == "CLP") {
        linear_programming_solver = LinearProgrammingSolver::CLP;
    } else if (token == "cplex" || token == "Cplex" || token == "CPLEX") {
        linear_programming_solver = LinearProgrammingSolver::CPLEX;
    } else if (token == "xpress" || token == "Xpress" || token == "XPRESS") {
        linear_programming_solver = LinearProgrammingSolver::Xpress;
    } else  {
        in.setstate(std::ios_base::failbit);
    }
    return in;
}

std::ostream& operator<<(std::ostream &os, LinearProgrammingSolver linear_programming_solver)
{
    switch (linear_programming_solver) {
    case LinearProgrammingSolver::CLP: {
        os << "CLP";
        break;
    } case LinearProgrammingSolver::CPLEX: {
        os << "CPLEX";
        break;
    } case LinearProgrammingSolver::Xpress: {
        os << "Xpress";
        break;
    }
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const Column& column)
{
    os << "objective coefficient: " << column.objective_coefficient << std::endl;
    os << "row indices:";
    for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos)
        os << " " << column.row_indices[row_pos];
    os << std::endl;
    os << "row coefficients:";
    for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_coefficients.size(); ++row_pos)
        os << " " << column.row_coefficients[row_pos];
    return os;
}

inline void display_initialize(Parameters& p, optimizationtools::Info& info)
{
    VER(info, std::left << std::setw(10) << "T (s)");
    if (p.objective_sense == ObjectiveSense::Min) {
        VER(info, std::left << std::setw(14) << "UB");
        VER(info, std::left << std::setw(14) << "LB");
    } else {
        VER(info, std::left << std::setw(14) << "LB");
        VER(info, std::left << std::setw(14) << "UB");
    }
    VER(info, std::left << std::setw(14) << "GAP");
    VER(info, std::left << std::setw(14) << "GAP (%)");
    VER(info, "");
    VER(info, std::endl);
}

inline void display(
        Value primal,
        Value dual,
        const std::stringstream& s,
        optimizationtools::Info& info)
{
    double t = (double)std::round(info.elapsed_time() * 10000) / 10000;
    VER(info, std::left << std::setw(10) << t);
    VER(info, std::left << std::setw(14) << primal);
    VER(info, std::left << std::setw(14) << dual);
    VER(info, std::left << std::setw(14) << std::abs(primal - dual));
    VER(info, std::left << std::setw(14) << 100.0 * std::abs(primal - dual) / std::max(std::abs(primal), std::abs(dual)));
    VER(info, s.str() << std::endl);
}

template <typename Output>
inline void display_end(
        const Output& output,
        optimizationtools::Info& info)
{
    double time = (double)std::round(info.elapsed_time() * 10000) / 10000;
    Value primal = output.solution_value;
    Value dual = output.bound;
    VER(info, "---" << std::endl
            << "Solution:             " << primal << std::endl
            << "Bound:                " << dual << std::endl
            << "Absolute gap:         " << std::abs(primal - dual) << std::endl
            << "Relative gap (%):     " << 100.0 * std::abs(primal - dual) / std::max(std::abs(primal), std::abs(dual)) << std::endl
            << "Total column number:  " << output.total_column_number << std::endl
            << "Added column number:  " << output.added_column_number << std::endl
            << "Total time (s):       " << time << std::endl);
}

inline bool is_feasible(
        const Parameters& parameters,
        std::vector<std::pair<ColIdx, Value>> solution)
{
    RowIdx m = parameters.row_lower_bounds.size();
    std::vector<RowIdx> row_values(m, 0.0);
    for (auto p: solution) {
        ColIdx col = p.first;
        Value val = p.second;
        const Column& column = parameters.columns[col];
        for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
            RowIdx row_index = column.row_indices[row_pos];
            Value row_coefficient = column.row_coefficients[row_pos];
            row_values[row_index] += val * row_coefficient;
        }
    }

    for (RowIdx row = 0; row < m; ++row) {
        if (row_values[row] < parameters.row_lower_bounds[row])
            return false;
        if (row_values[row] > parameters.row_upper_bounds[row])
            return false;
    }
    return true;
}

inline Value compute_value(
        const Parameters& parameters,
        std::vector<std::pair<ColIdx, Value>> solution)
{
    Value c = 0.0;
    for (auto p: solution) {
        ColIdx col = p.first;
        const Column& column = parameters.columns[col];
        c += column.objective_coefficient;
    }
    return c;
}

inline Value compute_reduced_cost(
        const Column& column,
        const std::vector<Value>& duals)
{
    Value reduced_cost = column.objective_coefficient;
    for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
        RowIdx row_index = column.row_indices[row_pos];
        Value row_coefficient = column.row_coefficients[row_pos];
        reduced_cost -= duals[row_index] * row_coefficient;
    }
    return reduced_cost;
}

inline Value norm(
        const std::vector<RowIdx>& new_rows,
        const std::vector<Value>& vector)
{
    Value res = 0;
    for (RowIdx row_pos = 0; row_pos < (RowIdx)new_rows.size(); ++row_pos) {
        RowIdx i = new_rows[row_pos];
        res += vector[i] * vector[i];
    }
    return std::sqrt(res);
}

inline Value norm(
        const std::vector<RowIdx>& new_rows,
        const std::vector<Value>& vector_1,
        const std::vector<Value>& vector_2)
{
    Value res = 0;
    for (RowIdx row_pos = 0; row_pos < (RowIdx)new_rows.size(); ++row_pos) {
        RowIdx i = new_rows[row_pos];
        res += (vector_2[i] - vector_1[i]) * (vector_2[i] - vector_1[i]);
    }
    return std::sqrt(res);
}

/***************************** Column Generation ******************************/

ColumnGenerationOutput columngeneration(
        Parameters& parameters,
        ColumnGenerationOptionalParameters optional_parameters)
{
    VER(optional_parameters.info, "*** columngeneration ***" << std::endl);
    VER(optional_parameters.info, "---" << std::endl);
    VER(optional_parameters.info, "Linear programming solver:                " << parameters.linear_programming_solver << std::endl);
    VER(optional_parameters.info, "Static Wentges smoothing parameter:       " << optional_parameters.static_wentges_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Static directional smoothing parameter:   " << optional_parameters.static_directional_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Self-adjusting Wentges smoothing:         " << optional_parameters.self_adjusting_wentges_smoothing << std::endl);
    VER(optional_parameters.info, "Automatic directional smoothing:          " << optional_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "Column generation iteration limit:        " << optional_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "---" << std::endl);
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
        if (parameters.column_lower_bound >= 0
                && parameters.row_coefficient_lower_bounds[row] >= 0
                && row_values[row] > parameters.row_upper_bounds[row]) {
            // Infeasible, what should we do?
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
    RowIdx new_row_number = row_pos;
    //std::cout << "new_row_number: " << new_row_number << std::endl;
    if (new_row_number == 0)
        return output;

    // Compute new row bounds.
    //std::cout << "Compute new row bounds..." << std::endl;
    std::vector<Value> new_row_lower_bounds(new_row_number);
    std::vector<Value> new_row_upper_bounds(new_row_number);
    for (RowIdx row = 0; row < new_row_number; ++row) {
        new_row_lower_bounds[row] = parameters.row_lower_bounds[new_rows[row]];
        new_row_upper_bounds[row] = parameters.row_upper_bounds[new_rows[row]];
        //std::cout << "row " << row << " lb " << new_row_lower_bounds[row] << " ub " << new_row_upper_bounds[row] << std::endl;
    }

    // Initialize solver
    //std::cout << "Initialize solver..." << std::endl;
    std::unique_ptr<ColumnGenerationSolver> solver = NULL;
#if CPLEX_FOUND
    if (parameters.linear_programming_solver == LinearProgrammingSolver::CPLEX)
        solver = std::unique_ptr<ColumnGenerationSolver>(
                new ColumnGenerationSolverCplex(
                    parameters.objective_sense,
                    new_row_lower_bounds,
                    new_row_upper_bounds));
#endif
#if COINOR_FOUND
    if (parameters.linear_programming_solver == LinearProgrammingSolver::CLP)
        solver = std::unique_ptr<ColumnGenerationSolver>(
                new ColumnGenerationSolverClp(
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
    RowIdx dummy_column_number = 0;
    for (RowIdx row = 0; row < new_row_number; ++row) {
        if (new_row_lower_bounds[row] > 0) {
            solver_column_indices.push_back(-1);
            dummy_column_number++;
            solver->add_column(
                    {row},
                    {new_row_lower_bounds[row]},
                    parameters.dummy_column_objective_coefficient,
                    parameters.column_lower_bound,
                    parameters.column_upper_bound);
        }
        if (new_row_upper_bounds[row] < 0) {
            solver_column_indices.push_back(-1);
            dummy_column_number++;
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
        std::vector<RowIdx> ri(new_row_number);
        std::vector<Value> rc(new_row_number);
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
                    && row_values[i] == parameters.row_upper_bounds[i]) {
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
    for (output.iteration_number = 1;; output.iteration_number++) {
        // Solve LP
        auto start_lpsolve = std::chrono::high_resolution_clock::now();
        solver->solve();
        auto end_lpsolve = std::chrono::high_resolution_clock::now();
        auto time_span_lpsolve = std::chrono::duration_cast<std::chrono::duration<double>>(end_lpsolve - start_lpsolve);
        output.time_lpsolve += time_span_lpsolve.count();
        VER(optional_parameters.info,
                "it " << std::setw(8) << output.iteration_number
                << " | T " << std::setw(10) << optional_parameters.info.elapsed_time()
                << " | OBJ " << std::setw(10) << c0 + solver->objective()
                << " | COL " << std::setw(10) << output.added_column_number
                << std::endl);

        // Check time.
        if (!optional_parameters.info.check_time())
            break;
        // Check 'end' parameter.
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;
        // Check iteration limit.
        if (optional_parameters.iteration_limit != -1
                && output.iteration_number > optional_parameters.iteration_limit)
            break;

        // Search for new columns.
        std::vector<Column> all_columns;
        std::vector<Column> new_columns;
        duals_in = duals_sep; // The last shall be the first.
        // Get duals from linear programming solver.
        for (RowIdx row_pos = 0; row_pos < new_row_number; ++row_pos)
            duals_out[new_rows[row_pos]] = solver->dual(row_pos);
        //std::cout << "alpha " << alpha << std::endl;
        for (Counter k = 1; ; ++k) { // Mispricing number.
            // Update global mispricing number.
            if (k > 1)
                output.mispricing_number++;
            // Compute separation point.
            double alpha_cur = std::max(0.0, 1 - k * (1 - alpha) - TOL);
            double beta = optional_parameters.static_directional_smoothing_parameter;
            //std::cout << "alpha_cur " << alpha_cur << std::endl;
            if (output.iteration_number == 1
                    || norm(new_rows, duals_in, duals_out) == 0 // Shouldn't happen, but happens with Cplex.
                    || k > 1) { // No directional smoothing.
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
            output.pricing_number++;
            if (alpha_cur == 0 && beta == 0)
                output.no_stab_pricing_number++;
            // Look for negative reduced cost columns.
            for (const Column& column: all_columns) {
                Value rc = compute_reduced_cost(column, duals_out);
                if (parameters.objective_sense == ObjectiveSense::Min
                        && rc <= 0 - TOL)
                    new_columns.push_back(column);
                if (parameters.objective_sense == ObjectiveSense::Max
                        && rc >= 0 + TOL)
                    new_columns.push_back(column);
            }
            if (!new_columns.empty() || (alpha_cur == 0.0 && beta == 0.0)) {
                if (k == 1)
                    output.first_try_pricing_number++;
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
        for (RowIdx i: new_rows)
            subgradient[i]
                = std::min(0.0, new_row_upper_bounds[i] - lagrangian_constraint_values[i])
                + std::max(0.0, new_row_lower_bounds[i] - lagrangian_constraint_values[i]);

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
            output.added_column_number++;
            // Add new column to the local LP solver.
            std::vector<RowIdx> ri(new_row_number);
            std::vector<Value> rc(new_row_number);
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

    double time = (double)std::round(optional_parameters.info.elapsed_time() * 10000) / 10000;
    VER(optional_parameters.info, "---" << std::endl
            << "Solution:                 " << output.solution_value << std::endl
            << "Iteration number:         " << output.iteration_number << std::endl
            << "Total column number:      " << parameters.columns.size() << std::endl
            << "Added column number:      " << output.added_column_number << std::endl
            << "Pricing number:           " << output.pricing_number << std::endl
            << "1st try pricing number:   " << output.first_try_pricing_number << std::endl
            << "Mispricing number:        " << output.mispricing_number << std::endl
            << "No stab. pricing number:  " << output.no_stab_pricing_number << std::endl
            << "Time LP solve (s):        " << output.time_lpsolve << std::endl
            << "Time pricing (s):         " << output.time_pricing << std::endl
            << "Time (s):                 " << time << std::endl);
    return output;
}

/*********************************** Greedy ***********************************/

GreedyOutput greedy(
        Parameters& parameters,
        GreedyOptionalParameters optional_parameters)
{
    VER(optional_parameters.info, "*** greedy ***" << std::endl);
    VER(optional_parameters.info, "---" << std::endl);
    VER(optional_parameters.info, "Linear programming solver:                " << parameters.linear_programming_solver << std::endl);
    VER(optional_parameters.info, "Static Wentges smoothing parameter:       " << optional_parameters.columngeneration_parameters.static_wentges_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Static directional smoothing parameter:   " << optional_parameters.columngeneration_parameters.static_directional_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Self-adjusting Wentges smoothing:         " << optional_parameters.columngeneration_parameters.self_adjusting_wentges_smoothing << std::endl);
    VER(optional_parameters.info, "Automatic directional smoothing:          " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "Column generation iteration limit:        " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    GreedyOutput output;
    output.solution_value = (parameters.objective_sense == ObjectiveSense::Min)?
        std::numeric_limits<Value>::infinity():
        -std::numeric_limits<Value>::infinity();
    output.bound = (parameters.objective_sense == ObjectiveSense::Min)?
        -std::numeric_limits<Value>::infinity():
        std::numeric_limits<Value>::infinity();
    std::vector<std::pair<ColIdx, Value>> fixed_columns;

    for (;;) {
        // Check time
        if (!optional_parameters.info.check_time())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;

        ColumnGenerationOptionalParameters columngeneration_parameters
            = optional_parameters.columngeneration_parameters;
        columngeneration_parameters.fixed_columns = &fixed_columns;
        columngeneration_parameters.end = optional_parameters.end;
        columngeneration_parameters.info.set_timelimit(optional_parameters.info.remaining_time());
        //columngeneration_parameters.info.set_verbose(true);
        auto output_columngeneration = columngeneration(
                parameters,
                columngeneration_parameters);
        output.time_lpsolve += output_columngeneration.time_lpsolve;
        output.time_pricing += output_columngeneration.time_pricing;
        output.added_column_number += output_columngeneration.added_column_number;
        if (!optional_parameters.info.check_time())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;
        if (output_columngeneration.solution.size() == 0)
            break;

        ColIdx col_best = -1;
        Value val_best = -1;
        Value diff_best = -1;
        Value bp_best = -1;
        for (auto p: output_columngeneration.solution) {
            ColIdx col = p.first;
            Value val = p.second;
            Value bp = parameters.columns[col].branching_priority;
            if (floor(val) != 0) {
                if (col_best == -1
                        || bp_best < bp
                        || (bp_best == bp && diff_best > val - floor(val))) {
                    col_best = col;
                    val_best = floor(val);
                    diff_best = val - floor(val);
                    bp_best = bp;
                }
            }
            if (ceil(val) != 0) {
                if (col_best == -1
                        || bp_best < bp
                        || (bp_best == bp && diff_best > ceil(val) - val)) {
                    col_best = col;
                    val_best = ceil(val);
                    diff_best = ceil(val) - val;
                    bp_best = bp;
                }
            }
        }
        if (col_best == -1)
            break;

        // Update bound.
        if (fixed_columns.size() == 0) {
            Counter cg_it_limit = optional_parameters.columngeneration_parameters.iteration_limit;
            if (cg_it_limit == -1 || (output_columngeneration.iteration_number < cg_it_limit))
                output.bound = output_columngeneration.solution_value;
        }
        // Update fixed columns.
        fixed_columns.push_back({col_best, val_best});
        // Update solution.
        if (is_feasible(parameters, fixed_columns)) {
            if (output.solution.size() == 0) {
                output.solution = fixed_columns;
                output.solution_value = compute_value(parameters, fixed_columns);
            } else {
                Value solution_value = compute_value(parameters, fixed_columns);
                if (parameters.objective_sense == ObjectiveSense::Min
                        && output.solution_value > solution_value) {
                    output.solution = fixed_columns;
                    output.solution_value = solution_value;
                }
                if (parameters.objective_sense == ObjectiveSense::Max
                        && output.solution_value < solution_value) {
                    output.solution = fixed_columns;
                    output.solution_value = solution_value;
                }
            }
        }
    }

    output.total_column_number = parameters.columns.size();
    display_end(output, optional_parameters.info);
    return output;
}

/************************* Limited Discrepancy Search *************************/

struct LimitedDiscrepancySearchNode
{
    std::shared_ptr<LimitedDiscrepancySearchNode> father = nullptr;
    ColIdx col = -1;
    Value value = 0;
    Value discrepancy = 0;
    Value value_sum = 1;
    ColIdx depth = 0;
};

LimitedDiscrepancySearchOutput limiteddiscrepancysearch(
        Parameters& parameters,
        LimitedDiscrepancySearchOptionalParameters optional_parameters)
{
    VER(optional_parameters.info, "*** limiteddiscrepancysearch ***" << std::endl);
    VER(optional_parameters.info, "---" << std::endl);
    VER(optional_parameters.info, "Linear programming solver:                " << parameters.linear_programming_solver << std::endl);
    VER(optional_parameters.info, "Discrepancy limit:                        " << optional_parameters.discrepancy_limit << std::endl);
    VER(optional_parameters.info, "Static Wentges smoothing parameter:       " << optional_parameters.columngeneration_parameters.static_wentges_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Static directional smoothing parameter:   " << optional_parameters.columngeneration_parameters.static_directional_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Self-adjusting Wentges smoothing:         " << optional_parameters.columngeneration_parameters.self_adjusting_wentges_smoothing << std::endl);
    VER(optional_parameters.info, "Automatic directional smoothing:          " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "Column generation iteration limit:        " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "---" << std::endl);

    LimitedDiscrepancySearchOutput output;
    output.solution_value = (parameters.objective_sense == ObjectiveSense::Min)?
        std::numeric_limits<Value>::infinity():
        -std::numeric_limits<Value>::infinity();
    output.bound = (parameters.objective_sense == ObjectiveSense::Min)?
        -std::numeric_limits<Value>::infinity():
        std::numeric_limits<Value>::infinity();
    display_initialize(parameters, optional_parameters.info);

    // Nodes
    auto comp = [](
            const std::shared_ptr<LimitedDiscrepancySearchNode>& node_1,
            const std::shared_ptr<LimitedDiscrepancySearchNode>& node_2)
    {
        if (node_1->discrepancy != node_2->discrepancy)
            return node_1->discrepancy < node_2->discrepancy;
        return node_1->value_sum > node_2->value_sum;
    };
    std::multiset<std::shared_ptr<LimitedDiscrepancySearchNode>, decltype(comp)> nodes(comp);

    // Root node.
    auto root = std::make_shared<LimitedDiscrepancySearchNode>();
    nodes.insert(root);

    while (!nodes.empty()) {
        output.node_number++;
        //std::cout << "nodes.size() " << nodes.size() << std::endl;

        // Check time.
        if (!optional_parameters.info.check_time())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;
        if (std::abs(output.solution_value - output.bound) < TOL)
            break;

        // Get node
        auto node = *nodes.begin();
        nodes.erase(nodes.begin());
        // Check discrepancy limit.
        if (node->discrepancy > optional_parameters.discrepancy_limit)
            break;

        std::vector<std::pair<ColIdx, Value>> fixed_columns;
        auto node_tmp = node;
        std::vector<uint8_t> tabu(parameters.columns.size(), 0);
        while (node_tmp->father != NULL) {
            if (node_tmp->value != 0)
                fixed_columns.push_back({node_tmp->col, node_tmp->value});
            tabu[node_tmp->col] = 1;
            node_tmp = node_tmp->father;
        }
        //std::cout
        //    << "t " << optional_parameters.info.elapsed_time()
        //    << " node " << output.node_number
        //    << " / " << nodes.size()
        //    << " diff " << node->discrepancy
        //    << " depth " << node->depth
        //    << " col " << node->col
        //    << " value " << node->value
        //    << " value_sum " << node->value_sum
        //    << " fixed_columns.size() " << fixed_columns.size()
        //    << std::endl;

        // Run column generation
        ColumnGenerationOptionalParameters columngeneration_parameters
            = optional_parameters.columngeneration_parameters;
        columngeneration_parameters.fixed_columns = &fixed_columns;
        columngeneration_parameters.end = optional_parameters.end;
        columngeneration_parameters.info.set_timelimit(optional_parameters.info.remaining_time());
        //columngeneration_parameters.info.set_verbose(true);
        auto output_columngeneration = columngeneration(
                parameters,
                columngeneration_parameters);
        output.time_lpsolve += output_columngeneration.time_lpsolve;
        output.time_pricing += output_columngeneration.time_pricing;
        output.added_column_number += output_columngeneration.added_column_number;
        //std::cout << "bound " << output_columngeneration.solution_value << std::endl;
        if (!optional_parameters.info.check_time())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;
        if (node->depth == 0) {
            Counter cg_it_limit = optional_parameters.columngeneration_parameters.iteration_limit;
            if (cg_it_limit == -1 || (output_columngeneration.iteration_number < cg_it_limit)) {
                output.bound = output_columngeneration.solution_value;
                display(output.solution_value, output.bound, std::stringstream("root node"), optional_parameters.info);
                optional_parameters.new_bound_callback(output);
            }
        }
        if (output_columngeneration.solution.size() == 0) {
            continue;
        }

        // Check bound
        if (output.solution.size() > 0) {
            if (parameters.objective_sense == ObjectiveSense::Min
                    && output.solution_value <= output_columngeneration.solution_value + TOL)
                continue;
            if (parameters.objective_sense == ObjectiveSense::Max
                    && output.solution_value >= output_columngeneration.solution_value - TOL)
                continue;
        }

        // Compute next column to branch on.
        ColIdx col_best = -1;
        Value val_best = -1;
        Value diff_best = -1;
        Value bp_best = -1;
        for (auto p: output_columngeneration.solution) {
            ColIdx col = p.first;
            if (col < (ColIdx)tabu.size() && tabu[col] == 1)
                continue;
            Value val = p.second;
            Value bp = parameters.columns[col].branching_priority;
            if (floor(val) != 0) {
                if (col_best == -1
                        || bp_best < bp
                        || (bp_best == bp && diff_best > val - floor(val))) {
                    col_best = col;
                    val_best = floor(val);
                    diff_best = val - floor(val);
                    bp_best = bp;
                }
            }
            if (ceil(val) != 0) {
                if (col_best == -1
                        || bp_best < bp
                        || (bp_best == bp && diff_best > ceil(val) - val)) {
                    col_best = col;
                    val_best = ceil(val);
                    diff_best = ceil(val) - val;
                    bp_best = bp;
                }
            }
        }
        //std::cout << "col_best " << col_best
        //    << " val_best " << val_best
        //    << " diff_best " << diff_best
        //    << std::endl;
        if (col_best == -1)
            continue;

        // Create children.
        for (Value value = parameters.column_lower_bound; value <= parameters.column_upper_bound; ++value) {
            auto child = std::make_shared<LimitedDiscrepancySearchNode>();
            child->father      = node;
            child->col         = col_best;
            child->value       = value;
            child->value_sum   = node->value_sum + value;
            child->discrepancy = node->discrepancy + abs(val_best - value);
            child->depth       = node->depth + 1;
            nodes.insert(child);

            // Update best solution.
            fixed_columns.push_back({col_best, value});
            if (is_feasible(parameters, fixed_columns)) {
                if (output.solution.size() == 0) {
                    output.solution = fixed_columns;
                    output.solution_value = compute_value(parameters, fixed_columns);
                    output.solution_discrepancy = node->discrepancy;
                    //std::cout << "New best solution value " << output.solution_value << std::endl;
                    std::stringstream ss;
                    ss << "node " << output.node_number << " discrepancy " << output.solution_discrepancy;
                    display(output.solution_value, output.bound, ss, optional_parameters.info);
                    optional_parameters.new_bound_callback(output);
                } else {
                    Value solution_value = compute_value(parameters, fixed_columns);
                    if (parameters.objective_sense == ObjectiveSense::Min
                            && output.solution_value > solution_value) {
                        //std::cout << "New best solution value " << solution_value << std::endl;
                        output.solution = fixed_columns;
                        output.solution_value = solution_value;
                        output.solution_discrepancy = node->discrepancy;
                        std::stringstream ss;
                        ss << "node " << output.node_number << " discrepancy " << output.solution_discrepancy;
                        display(output.solution_value, output.bound, ss, optional_parameters.info);
                        optional_parameters.new_bound_callback(output);
                    }
                    if (parameters.objective_sense == ObjectiveSense::Max
                            && output.solution_value < solution_value) {
                        output.solution = fixed_columns;
                        output.solution_value = solution_value;
                        output.solution_discrepancy = node->discrepancy;
                        std::stringstream ss;
                        ss << "node " << output.node_number << " discrepancy " << output.solution_discrepancy;
                        display(output.solution_value, output.bound, ss, optional_parameters.info);
                        optional_parameters.new_bound_callback(output);
                    }
                }
            }
            fixed_columns.pop_back();
        }

    }

    output.total_column_number = parameters.columns.size();
    display_end(output, optional_parameters.info);
    VER(optional_parameters.info, "Node number:          " << output.node_number << std::endl);
    return output;
}

/*************************** Heuristic Tree Search ****************************/

struct HeuristicTreeSearchNode
{
    std::shared_ptr<HeuristicTreeSearchNode> father = nullptr;
    ColIdx col = -1;
    Value value = 0;
    Value discrepancy = 0;
    Value value_sum = 1;
    ColIdx depth = 0;
};

HeuristicTreeSearchOutput heuristictreesearch(
        Parameters& parameters,
        HeuristicTreeSearchOptionalParameters optional_parameters)
{
    VER(optional_parameters.info, "*** heuristictreesearch ***" << std::endl);
    VER(optional_parameters.info, "---" << std::endl);
    VER(optional_parameters.info, "Linear programming solver:                " << parameters.linear_programming_solver << std::endl);
    VER(optional_parameters.info, "Static Wentges smoothing parameter:       " << optional_parameters.columngeneration_parameters.static_wentges_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Static directional smoothing parameter:   " << optional_parameters.columngeneration_parameters.static_directional_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Self-adjusting Wentges smoothing:         " << optional_parameters.columngeneration_parameters.self_adjusting_wentges_smoothing << std::endl);
    VER(optional_parameters.info, "Automatic directional smoothing:          " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "Column generation iteration limit:        " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "---" << std::endl);

    HeuristicTreeSearchOutput output;
    output.solution_value = (parameters.objective_sense == ObjectiveSense::Min)?
        std::numeric_limits<Value>::infinity():
        -std::numeric_limits<Value>::infinity();
    output.bound = (parameters.objective_sense == ObjectiveSense::Min)?
        -std::numeric_limits<Value>::infinity():
        std::numeric_limits<Value>::infinity();
    display_initialize(parameters, optional_parameters.info);

    output.total_column_number = parameters.columns.size();
    display_end(output, optional_parameters.info);
    return output;
}

}

