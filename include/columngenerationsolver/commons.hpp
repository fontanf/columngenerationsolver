#pragma once

#include "optimizationtools/utils/output.hpp"
#include "optimizationtools/utils/utils.hpp"
#include "optimizationtools/containers/indexed_map.hpp"

#include <vector>
#include <cstdint>
#include <iomanip>

namespace columngenerationsolver
{

using Counter = int64_t;
using ColIdx = int64_t;
using RowIdx = int64_t;
using Value = double;

enum class VariableType { Continuous, Integer };

struct LinearTerm
{
    /** Row index. */
    RowIdx row;

    /** Coefficient. */
    Value coefficient;
};

/**
 * Structure for a column.
 */
struct Column
{
    /** Column name. */
    std::string name;

    /** Type. */
    VariableType type = VariableType::Integer;

    /** Lower bound. */
    Value lower_bound = 0.0;

    /** Upper bound. */
    Value upper_bound = std::numeric_limits<Value>::infinity();

    /** Coefficient in the objective. */
    Value objective_coefficient = 0;

    /** Row indices. */
    std::vector<LinearTerm> elements;

    /** Branching priority. */
    Value branching_priority = 0;

    /**
     * Extra information.
     *
     * This field may be used to retrieve the real solution from the column.
     * For example, if a column represent a path, the order in which the
     * elements of the path are visited may be stored in this attribute.
     */
    std::shared_ptr<void> extra;
};

inline std::ostream& operator<<(
        std::ostream& os,
        const Column& column)
{
    os << "name: " << column.name << std::endl;
    os << "objective coefficient: " << column.objective_coefficient << std::endl;
    os << "lower bound: " << column.lower_bound << std::endl;
    os << "upper bound: " << column.upper_bound << std::endl;
    os << "row indices:";
    for (RowIdx row_pos = 0;
            row_pos < (RowIdx)column.elements.size();
            ++row_pos) {
        os << " " << column.elements[row_pos].row;
    }
    os << std::endl;
    os << "row coefficients:";
    for (RowIdx row_pos = 0;
            row_pos < (RowIdx)column.elements.size();
            ++row_pos) {
        os << " " << column.elements[row_pos].coefficient;
    }
    return os;
}

/**
 * Structure for a row.
 */
struct Row
{
    /** Row name. */
    std::string name;

    /** Lower bounds of the constraints. */
    Value lower_bound = 0.0;

    /** Upper bounds of the constraints. */
    Value upper_bound = 0.0;

    /**
     * Lower bounds of the coefficicients of the variables to generate for each
     * constraint.
     */
    Value coefficient_lower_bound = 0.0;

    /**
     * Upper bounds of the coefficicients of the variables to generate for each
     * constraint.
     */
    Value coefficient_upper_bound = 1.0;
};

/**
 * Interface for the pricing problem solver.
 */
class PricingSolver
{

public:

    virtual ~PricingSolver() { }

    struct PricingOutput
    {
        std::vector<std::shared_ptr<const Column>> columns;

        Value overcost = std::numeric_limits<Value>::infinity();
    };

    virtual std::vector<std::shared_ptr<const Column>> initialize_pricing(
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns) = 0;

    virtual PricingOutput solve_pricing(
            const std::vector<Value>& duals) = 0;
};

/**
 * Structure for the (expenential) model.
 */
struct Model
{
    /** Objective sense. */
    optimizationtools::ObjectiveDirection objective_sense = optimizationtools::ObjectiveDirection::Minimize;

    /** Constraints. */
    std::vector<Row> rows;

    /** Solver of the pricing problem. */
    std::unique_ptr<PricingSolver> pricing_solver = NULL;

    /** Column which are not dynamically generated. */
    std::vector<std::shared_ptr<const Column>> static_columns;


    void check_column(
            const std::shared_ptr<const Column>& column) const
    {
        for (const LinearTerm& element: column->elements) {
            if (element.row < 0 || element.row >= rows.size()) {
                std::stringstream ss;
                ss << "Column check failed." << std::endl
                    << "Column:" << std::endl << *column << std::endl
                    << "Invalid row index." << std::endl;
                throw std::runtime_error(ss.str());
            }
        }
    }

    void check_generated_column(
            const std::shared_ptr<const Column>& column) const
    {
        if (column->lower_bound != 0) {
            std::stringstream ss;
            ss << "Generated column check failed." << std::endl
                << "Column:" << std::endl << *column << std::endl
                << "A generated column must have a zero lower bound." << std::endl;
            throw std::runtime_error(ss.str());
        }
        if (column->upper_bound != std::numeric_limits<Value>::infinity()) {
            std::stringstream ss;
            ss << "Generated column check failed." << std::endl
                << "Column:" << std::endl << *column << std::endl
                << "A generated column must have an infinite upper bound." << std::endl;
            throw std::runtime_error(ss.str());
        }
        check_column(column);
    }


    virtual void format(
            std::ostream& os,
            int verbosity_level = 1) const
    {
        if (verbosity_level >= 1) {
            os
                << "Objective sense:           " << ((objective_sense == optimizationtools::ObjectiveDirection::Minimize)? "Minimize": "Maximize") << std::endl
                << "Number of constraints:     " << rows.size() << std::endl
                << "Number of static columns:  " << static_columns.size() << std::endl
                ;
        }

        if (verbosity_level >= 2) {
            os
                << std::endl
                << std::setw(12) << "Row"
                << std::setw(36) << "Name"
                << std::setw(12) << "Lower"
                << std::setw(12) << "Upper"
                << std::endl
                << std::setw(12) << "---"
                << std::setw(36) << "-----"
                << std::setw(12) << "-----"
                << std::setw(12) << "-----"
                << std::endl;
            for (RowIdx row_id = 0;
                    row_id < (RowIdx)rows.size();
                    ++row_id) {
                const Row& row = this->rows[row_id];
                os
                    << std::setw(12) << row_id
                    << std::setw(36) << row.name
                    << std::setw(12) << row.lower_bound
                    << std::setw(12) << row.upper_bound
                    << std::endl;
            }
        }

        if (verbosity_level >= 3) {

            std::vector<std::vector<std::pair<const Column*, Value>>> row_elements(this->rows.size());
            for (const auto& column: static_columns) {
                for (const LinearTerm& element: column->elements)
                    row_elements[element.row].push_back({column.get(), element.coefficient});
            }
            for (RowIdx row_id = 0;
                    row_id < (RowIdx)rows.size();
                    ++row_id) {
                const Row& row = this->rows[row_id];
                os << "- " << row_id << " " << row.name << ":";
                if (row.upper_bound != std::numeric_limits<Value>::infinity()
                        && row.lower_bound != -std::numeric_limits<Value>::infinity()) {
                    os << " " << row.lower_bound << " <=";
                }

                bool first = true;
                for (const auto& p: row_elements[row_id]) {
                    const Column* column = p.first;
                    Value coef = p.second;
                    if (coef == 0)
                        continue;
                    if (first) {
                        if (coef == 1) {
                            os << " " << column->name;
                        } else if (coef == -1) {
                            os << " - " << column->name;
                        } else if (coef > 0) {
                            os << " " << coef << " " << column->name;
                        } else {
                            os << " - " << -coef << " " << column->name;
                        }
                    } else {
                        if (coef == 1) {
                            os << " + " << column->name;
                        } else if (coef == -1) {
                            os << " - " << column->name;
                        } else if (coef > 0) {
                            os << " + " << coef << " " << column->name;
                        } else {
                            os << " - " << -coef << " " << column->name;
                        }
                    }
                    first = false;
                }

                if (row.upper_bound != std::numeric_limits<Value>::infinity()) {
                    os << " <= " << row.upper_bound;
                } else {
                    os << " >= " << row.lower_bound;
                }
                os << std::endl;
            }
            os << std::endl;
        }
    }
};

class ColumnMap
{

public:

    /** Get columns. */
    const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& columns() const { return columns_; };

    bool contains(
            const std::shared_ptr<const Column>& column)
    {
        return (columns_map_.find(column) != columns_map_.end());
    }

    Value get_column_value(
            const std::shared_ptr<const Column>& column,
            Value default_value = 0) const
    {
        auto it = columns_map_.find(column);
        if (it == columns_map_.end())
            return default_value;
        Counter pos = it->second;
        return columns_[pos].second;
    }

    /** Add a column to the solution. */
    void set_column_value(
            const std::shared_ptr<const Column>& column,
            Value value)
    {
        if (columns_map_.find(column) == columns_map_.end()) {
            columns_map_[column] = columns_.size();
            columns_.push_back({column, value});
        } else {
            Counter pos = columns_map_[column];
            columns_[pos].second = value;
        }
    }

    void max_column_value(
            const std::shared_ptr<const Column>& column,
            Value value)
    {
        if (columns_map_.find(column) == columns_map_.end()) {
            columns_map_[column] = columns_.size();
            columns_.push_back({column, value});
        } else {
            Counter pos = columns_map_[column];
            if (columns_[pos].second < value)
                columns_[pos].second = value;
        }
    }

private:

    /*
     * Private methods
     */

    /*
     * Private attributes
     */

    /** Columns. */
    std::vector<std::pair<std::shared_ptr<const Column>, Value>> columns_;

    /** Map of columns to position in solution.columns_. */
    std::unordered_map<std::shared_ptr<const Column>, Counter> columns_map_;

};

/**
 * Solution class.
 */
class Solution
{

public:

    /** Get model. */
    const Model& model() const { return *model_; }

    /** Return 'true' iff the solution is feasible. */
    bool feasible() const { return feasible_; }

    /**
     * Return 'true' iff the solution is feasible for the constraints (but not
     * necessarily for the variable integrality).
     */
    bool feasible_relaxation() const { return feasible_relaxation_; }

    /** Get the objective value of the solution. */
    Value objective_value() const { return objective_value_; }

    /** Get columns. */
    const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& columns() const { return columns_; };

    /*
     * Export
     */

    /** Export solution characteristics to a JSON structure. */
    nlohmann::json to_json() const;

    /** Write a formatted output of the instance to a stream. */
    void format(
            std::ostream& os,
            int verbosity_level = 1) const
    {
        if (verbosity_level >= 1) {
            os
                << "Feasible:           " << feasible() << std::endl
                << "Value:              " << objective_value() << std::endl
                << "Number of columns:  " << columns_.size() << std::endl
                ;
        }

        if (verbosity_level >= 2) {
            os << std::endl
                << std::setw(12) << "Row"
                << std::setw(12) << "Lower"
                << std::setw(12) << "Value"
                << std::setw(12) << "Upper"
                << std::setw(12) << "Feasible"
                << std::endl
                << std::setw(12) << "---"
                << std::setw(12) << "-----"
                << std::setw(12) << "-----"
                << std::setw(12) << "-----"
                << std::setw(12) << "--------"
                << std::endl;
            for (RowIdx row_id = 0;
                    row_id < (RowIdx)this->model().rows.size();
                    ++row_id) {
                const Row& row = this->model().rows[row_id];
                bool infeasible = (row_values_[row_id] > model_->rows[row_id].upper_bound + FFOT_TOL)
                    || (row_values_[row_id] < model_->rows[row_id].lower_bound - FFOT_TOL);
                os
                    << std::setw(12) << row_id
                    << std::setw(12) << row.lower_bound
                    << std::setw(12) << row_values_[row_id]
                    << std::setw(12) << row.upper_bound
                    << std::setw(12) << !infeasible
                    << std::endl;
            }

            os
                << std::endl
                << std::setw(12) << "Type"
                << std::setw(12) << "Value"
                << std::setw(12) << "Integral"
                << std::endl
                << std::setw(12) << "----"
                << std::setw(12) << "-----"
                << std::setw(12) << "--------"
                << std::endl;
            for (const auto& p: this->columns()) {
                Value value = p.second;
                Value fractionality = std::fabs(value - std::round(value));
                bool integral = (p.first->type == VariableType::Continuous)
                    || !(fractionality > FFOT_TOL);
                os
                    << std::setw(12) << ((p.first->type == VariableType::Continuous)? "C": "I")
                    << std::setw(12) << p.second
                    << std::setw(12) << integral
                    << std::endl;
            }
        }
    }

private:

    /** Constructor. */
    Solution() { }

    /** Model. */
    const Model* model_;

    /** Feasible. */
    bool feasible_;

    /**
     * Feasible regarding the constraints but the regarding the column
     * integrality.
     */
    bool feasible_relaxation_;

    /** Objective value. */
    Value objective_value_;

    /** Row values. */
    std::vector<Value> row_values_;

    /** Columns. */
    std::vector<std::pair<std::shared_ptr<const Column>, Value>> columns_;

    friend class SolutionBuilder;

};

class SolutionBuilder
{

public:

    /** Constructor. */
    SolutionBuilder() { }

    /** Set the model of the solution. */
    SolutionBuilder& set_model(const Model& model) { solution_.model_ = &model; return *this; }

    /** Add a column to the solution. */
    void add_column(
            const std::shared_ptr<const Column>& column,
            Value value)
    {
        if (value == 0)
            return;
        if (columns_map_.find(column) == columns_map_.end()) {
            columns_map_[column] = solution_.columns_.size();
            solution_.columns_.push_back({column, value});
        } else {
            Counter pos = columns_map_[column];
            solution_.columns_[pos].second = solution_.columns_[pos].second + value;
        }
    }

    /** Build. */
    Solution build()
    {
        compute_feasible();
        compute_objective_value();

        return std::move(solution_);
    }

private:

    /*
     * Private methods
     */

    /** Compute the feasibility of the solution. */
    void compute_feasible()
    {
        solution_.row_values_ = std::vector<Value>(solution_.model_->rows.size(), 0.0);
        for (const auto& p: solution_.columns_) {
            const Column& column = *p.first;
            Value column_value = p.second;
            for (const LinearTerm& element: column.elements) {
                solution_.row_values_[element.row] += column_value * element.coefficient;
            }
        }

        solution_.feasible_ = true;
        solution_.feasible_relaxation_ = true;
        for (RowIdx row = 0;
                row < (RowIdx)solution_.model_->rows.size();
                ++row) {
            if (solution_.row_values_[row] > solution_.model_->rows[row].upper_bound + FFOT_TOL) {
                //std::cout << "row " << row
                //    << " lb " << solution_.model_->rows[row].lower_bound
                //    << " val " << solution_.row_values_[row]
                //    << " ub " << solution_.model_->rows[row].upper_bound
                //    << std::endl;
                solution_.feasible_ = false;
            }
            if (solution_.row_values_[row] < solution_.model_->rows[row].lower_bound - FFOT_TOL) {
                //std::cout << "row " << row
                //    << " lb " << solution_.model_->rows[row].lower_bound
                //    << " val " << solution_.row_values_[row]
                //    << " ub " << solution_.model_->rows[row].upper_bound
                //    << std::endl;
                solution_.feasible_ = false;
                solution_.feasible_relaxation_ = false;
            }
        }

        for (const auto& p: solution_.columns_) {
            const Column& column = *(p.first);
            Value value = p.second;
            if (column.type == VariableType::Integer) {
                Value fractionality = std::fabs(value - std::round(value));
                if (fractionality > FFOT_TOL) {
                    solution_.feasible_ = false;
                }
            }
        }
    }

    /** Compute the objective value of the solution. */
    void compute_objective_value()
    {
        solution_.objective_value_ = 0.0;
        for (const auto& p: solution_.columns_) {
            const Column& column = *p.first;
            Value column_value = p.second;
            solution_.objective_value_ += column.objective_coefficient * column_value;
        }
    }

    /*
     * Private attributes
     */

    /** Solution. */
    Solution solution_;

    /** Map of columns to position in solution.columns_. */
    std::unordered_map<std::shared_ptr<const Column>, Counter> columns_map_;

};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Implementation ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline Value compute_reduced_cost(
        const Column& column,
        const std::vector<Value>& duals)
{
    Value reduced_cost = column.objective_coefficient;
    for (const LinearTerm& element: column.elements)
        reduced_cost -= duals[element.row] * element.coefficient;
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct Output: optimizationtools::Output
{
    /** Constructor. */
    Output(const Model& model):
        solution(SolutionBuilder().set_model(model).build()),
        bound((model.objective_sense == optimizationtools::ObjectiveDirection::Minimize)?
                -std::numeric_limits<Value>::infinity():
                +std::numeric_limits<Value>::infinity()),
        relaxation_solution(SolutionBuilder().set_model(model).build()) { }


    /** Solution. */
    Solution solution;

    /** Bound. */
    Value bound;

    /** Elapsed time. */
    double time = 0.0;

    /** Time spent solving the LP subproblems. */
    double time_lpsolve = 0.0;

    /** Time spent solving the pricing subproblems. */
    double time_pricing = 0.0;

    /** Objective coefficient of the dummy columns. */
    Value dummy_column_objective_coefficient;

    /** Number of column generation iterations. */
    Counter number_of_column_generation_iterations = 0;

    /** Columns generated during the algorithm. */
    std::vector<std::shared_ptr<const Column>> columns;

    /** Solution. */
    Solution relaxation_solution;


    std::string solution_value() const
    {
        return optimizationtools::solution_value(
                solution.model().objective_sense,
                solution.feasible(),
                solution.objective_value());
    }

    double absolute_optimality_gap() const
    {
        return optimizationtools::absolute_optimality_gap(
                solution.model().objective_sense,
                solution.feasible(),
                solution.objective_value(),
                bound);
    }

    double relative_optimality_gap() const
    {
       return optimizationtools::relative_optimality_gap(
               solution.model().objective_sense,
               solution.feasible(),
               solution.objective_value(),
               bound);
    }

    virtual nlohmann::json to_json() const
    {
        return {
            {"Value", solution_value()},
            {"Bound", bound},
            {"AbsoluteOptimalityGap", absolute_optimality_gap()},
            {"RelativeOptimalityGap", relative_optimality_gap()},
            {"Time", time},
            {"PricingTime", time_pricing},
            {"LpTime", time_lpsolve},
            {"NumberOfColumnGenerationIterations", number_of_column_generation_iterations},
            {"DummyColumnObjectiveCoefficient", dummy_column_objective_coefficient},
        };
    }

    virtual int format_width() const { return 30; }

    virtual void format(std::ostream& os) const
    {
        int width = format_width();
        os
            << std::setw(width) << std::left << "Value: " << solution_value() << std::endl
            << std::setw(width) << std::left << "Bound: " << bound << std::endl
            << std::setw(width) << std::left << "Absolute optimality gap: " << absolute_optimality_gap() << std::endl
            << std::setw(width) << std::left << "Relative optimality gap (%): " << relative_optimality_gap() * 100 << std::endl
            << std::setw(width) << std::left << "Time: " << time << std::endl
            << std::setw(width) << std::left << "Pricing time: " << time_pricing << std::endl
            << std::setw(width) << std::left << "Linear programming time: " << time_lpsolve << std::endl
            << std::setw(width) << std::left << "Dummy column coef.: " << dummy_column_objective_coefficient << std::endl
            << std::setw(width) << std::left << "Number of CG iterations: " << number_of_column_generation_iterations << std::endl
            << std::setw(width) << std::left << "Number of new columns: " << columns.size() << std::endl
            ;
    }
};

using NewSolutionCallback = std::function<void(const Output&)>;

enum class SolverName { CLP, Highs, CPLEX, Xpress, Knitro };

inline std::istream& operator>>(
        std::istream& in,
        SolverName& solver_name)
{
    std::string token;
    in >> token;
    if (token == "clp" || token == "CLP") {
        solver_name = SolverName::CLP;
    } else if (token == "highs" || token == "Highs" || token == "HIGHS") {
        solver_name = SolverName::Highs;
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
    } case SolverName::Highs: {
        os << "Highs";
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
    } else if (s == "highs" || s == "Highs" || s == "HIGHS") {
        return SolverName::Highs;
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

struct Parameters: optimizationtools::Parameters
{
    /** Callback function called when a new best solution is found. */
    NewSolutionCallback new_solution_callback = [](const Output&) { };

    /** Callback function called when a new best bound is found. */
    NewSolutionCallback new_bound_callback = [](const Output&) { };

    /** Objective coefficient of the dummy columns. */
    Value dummy_column_objective_coefficient = 1;

    /** Column pool. */
    std::vector<std::shared_ptr<const Column>> column_pool;

    /** Initial columns. */
    std::vector<std::shared_ptr<const Column>> initial_columns;

    /** Fixed columns. */
    std::vector<std::pair<std::shared_ptr<const Column>, Value>> fixed_columns;

    /**
     * Enable internal diving:
     * - 0: not enabled
     * - 1: enabled at the root node
     * - 2: enabled at all nodes
     */
    int internal_diving = 0;


    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = optimizationtools::Parameters::to_json();
        json.merge_patch({
                {"DummyColumnObjectiveCoefficient", dummy_column_objective_coefficient},
                {"NumberOfColumnsInTheColumnPool", column_pool.size()},
                {"NumberOfInitialColumns", initial_columns.size()},
                {"NumberOfFixedColumns", fixed_columns.size()},
                {"InternalDiving", internal_diving},
                });
        return json;
    }

    virtual int format_width() const override { return 41; }

    virtual void format(std::ostream& os) const override
    {
        optimizationtools::Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Dummy column coef.: " << dummy_column_objective_coefficient << std::endl
            << std::setw(width) << std::left << "Number of columns in the column pool: " << column_pool.size() << std::endl
            << std::setw(width) << std::left << "Number of initial columns: " << initial_columns.size() << std::endl
            << std::setw(width) << std::left << "Number of fixed columns: " << fixed_columns.size() << std::endl
            << std::setw(width) << std::left << "Internal diving: " << internal_diving << std::endl
            ;
    }
};

}
