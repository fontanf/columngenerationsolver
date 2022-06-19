#pragma once

#include "optimizationtools/utils/info.hpp"

#include <vector>
#include <cstdint>
#include <limits>
#include <set>

namespace columngenerationsolver
{

typedef int64_t Counter;
typedef int64_t ColIdx;
typedef int64_t RowIdx;
typedef double  Value;

enum class ObjectiveSense { Min, Max };

struct Column
{
    std::vector<RowIdx> row_indices;
    std::vector<Value> row_coefficients;
    Value objective_coefficient = 0;

    Value branching_priority = 0;
    std::shared_ptr<void> extra;
};

inline std::ostream& operator<<(std::ostream &os, const Column& column)
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
    Parameters(RowIdx number_of_rows):
        row_lower_bounds(number_of_rows),
        row_upper_bounds(number_of_rows),
        row_coefficient_lower_bounds(number_of_rows),
        row_coefficient_upper_bounds(number_of_rows) { }
    ObjectiveSense objective_sense = ObjectiveSense::Min;
    Value column_lower_bound;
    Value column_upper_bound;
    std::vector<Value> row_lower_bounds;
    std::vector<Value> row_upper_bounds;
    std::vector<Value> row_coefficient_lower_bounds;
    std::vector<Value> row_coefficient_upper_bounds;
    Value dummy_column_objective_coefficient;
    std::unique_ptr<PricingSolver> pricing_solver = NULL;
    std::vector<Column> columns;
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Implementation ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline void display_initialize(optimizationtools::Info& info)
{
    info.os()
            << std::setw(10) << "Time"
            << std::setw(14) << "Solution"
            << std::setw(14) << "Bound"
            << std::setw(14) << "Gap"
            << std::setw(10) << "Gap (%)"
            << std::setw(24) << "Comment" << std::endl
            << std::setw(10) << "----"
            << std::setw(14) << "--------"
            << std::setw(14) << "-----"
            << std::setw(14) << "---"
            << std::setw(10) << "-------"
            << std::setw(24) << "-------" << std::endl;
}

inline void display(
        Parameters& p,
        Value primal,
        Value dual,
        const std::stringstream& s,
        optimizationtools::Info& info)
{
    double t = info.elapsed_time();
    double gap = (p.objective_sense == ObjectiveSense::Min)?
        primal - dual:
        dual - primal;
    info.os()
            << std::setw(10) << std::fixed << std::setprecision(3) << t
            << std::setw(14) << std::fixed << std::setprecision(5) << primal
            << std::setw(14) << std::fixed << std::setprecision(5) << dual
            << std::setw(14) << std::fixed << std::setprecision(5) << gap
            << std::setw(10) << std::fixed << std::setprecision(2) << 100.0 * gap / std::max(std::abs(primal), std::abs(dual))
            << std::setw(24) << s.str()
            << std::endl;
}

template <typename Output>
inline void display_end(
        const Output& output,
        optimizationtools::Info& info)
{
    double t = info.elapsed_time();
    Value primal = output.solution_value;
    Value dual = output.bound;
    info.os() << std::defaultfloat
            << std::endl
            << "Final statistics" << std::endl
            << "----------------" << std::endl
            << "Solution:                 " << primal << std::endl
            << "Bound:                    " << dual << std::endl
            << "Absolute gap:             " << std::abs(primal - dual) << std::endl
            << "Relative gap (%):         " << 100.0 * std::abs(primal - dual) / std::max(std::abs(primal), std::abs(dual)) << std::endl
            << "Total number of columns:  " << output.total_number_of_columns << std::endl
            << "Number of columns added:  " << output.number_of_added_columns << std::endl
            << "Total time (s):           " << t << std::endl;
}

inline bool is_feasible(
        const Parameters& parameters,
        const std::vector<std::pair<ColIdx, Value>>& solution)
{
    RowIdx m = parameters.row_lower_bounds.size();
    std::vector<RowIdx> row_values(m, 0.0);
    for (const auto& p: solution) {
        const Column& column = parameters.columns[p.first];
        Value val = p.second;
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
        Value value = p.second;
        const Column& column = parameters.columns[col];
        c += column.objective_coefficient * value;
    }
    return c;
}

std::vector<std::pair<Column, Value> >const to_solution(
        const Parameters& parameters,
        const std::vector<std::pair<ColIdx, Value>>& columns)
{
    std::vector<std::pair<Column, Value>> solution;
    for (const auto& p: columns) {
        const Column column = parameters.columns[p.first];
        Value value = p.second;
        solution.push_back({column, value});
    }
    return solution;
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

}

