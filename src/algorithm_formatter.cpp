#include "columngenerationsolver/algorithm_formatter.hpp"

#include <iomanip>

using namespace columngenerationsolver;

void AlgorithmFormatter::start(
        const std::string& algorithm_name)
{
    output_.json["Parameters"] = parameters_.to_json();

    if (parameters_.verbosity_level == 0)
        return;
    *os_
        << "==========================================" << std::endl
        << "          ColumnGenerationSolver          " << std::endl
        << "==========================================" << std::endl
        << std::endl
        << "Model" << std::endl
        << "-----" << std::endl;
    model_.format(*os_, parameters_.verbosity_level);
    *os_
        << std::endl
        << "Algorithm" << std::endl
        << "---------" << std::endl
        << algorithm_name << std::endl
        << std::endl
        << "Parameters" << std::endl
        << "----------" << std::endl;
    parameters_.format(*os_);
}

void AlgorithmFormatter::print_column_generation_header()
{
    if (parameters_.verbosity_level == 0)
        return;
    *os_
        << std::endl
        << "Column generation" << std::endl
        << "-----------------" << std::endl
        << std::endl
        << std::right
        << std::setw(12) << "Time"
        << std::setw(12) << "Iteration"
        << std::setw(12) << "# columns"
        << std::setw(24) << "Value"
        << std::setw(24) << "Bound"
        << std::endl
        << std::setw(12) << "----"
        << std::setw(12) << "---------"
        << std::setw(12) << "---------"
        << std::setw(24) << "-----"
        << std::setw(24) << "-----"
        << std::endl;
}

void AlgorithmFormatter::print_column_generation_iteration(
            Counter iteration,
            ColIdx number_of_columns,
            double value,
            double bound)
{
    output_.time = parameters_.timer.elapsed_time();
    if (parameters_.verbosity_level == 0)
        return;
    std::streamsize precision = std::cout.precision();
    *os_
        << std::setw(12) << std::fixed << std::setprecision(3) << output_.time << std::defaultfloat << std::setprecision(precision)
        << std::setw(12) << iteration
        << std::setw(12) << number_of_columns
        << std::setw(24) << value
        << std::setw(24) << bound
        << std::endl;
}

void AlgorithmFormatter::print_header()
{
    if (parameters_.verbosity_level == 0)
        return;
    *os_
        << std::endl
        << "Tree search" << std::endl
        << "-----------" << std::endl
        << std::endl
        << std::right
        << std::setw(12) << "Time"
        << std::setw(12) << "Value"
        << std::setw(12) << "Bound"
        << std::setw(12) << "Gap"
        << std::setw(12) << "Gap (%)"
        << std::setw(32) << "Comment"
        << std::endl
        << std::setw(12) << "----"
        << std::setw(12) << "-----"
        << std::setw(12) << "-----"
        << std::setw(12) << "---"
        << std::setw(12) << "-------"
        << std::setw(32) << "-------"
        << std::endl;
}

void AlgorithmFormatter::print(
        const std::string& s)
{
    output_.time = parameters_.timer.elapsed_time();
    if (parameters_.verbosity_level == 0)
        return;
    std::streamsize precision = std::cout.precision();
    *os_
        << std::setw(12) << std::fixed << std::setprecision(3) << output_.time << std::defaultfloat << std::setprecision(precision)
        << std::setw(12) << output_.solution_value()
        << std::setw(12) << output_.bound
        << std::setw(12) << output_.absolute_optimality_gap()
        << std::setw(12) << std::fixed << std::setprecision(2) << output_.relative_optimality_gap() * 100 << std::defaultfloat << std::setprecision(precision)
        << std::setw(32) << s << std::endl;
}

void AlgorithmFormatter::update_solution(
        const Solution& solution)
{
    if (optimizationtools::is_solution_strictly_better(
                model_.objective_sense,
                output_.solution.feasible(),
                output_.solution.objective_value(),
                solution.feasible(),
                solution.objective_value())) {
        output_.time = parameters_.timer.elapsed_time();
        output_.solution = solution;
        output_.json["IntermediaryOutputs"].push_back(output_.to_json());
        parameters_.new_solution_callback(output_);
    }
}

void AlgorithmFormatter::update_bound(
        double bound)
{
    if (optimizationtools::is_bound_strictly_better(
                model_.objective_sense,
                output_.bound,
                bound)) {
        output_.time = parameters_.timer.elapsed_time();
        output_.bound = bound;
        output_.json["IntermediaryOutputs"].push_back(output_.to_json());
        parameters_.new_bound_callback(output_);
    }
}

void AlgorithmFormatter::end()
{
    output_.time = parameters_.timer.elapsed_time();
    output_.json["Output"] = output_.to_json();

    if (parameters_.verbosity_level == 0)
        return;
    *os_
        << std::endl
        << "Final statistics" << std::endl
        << "----------------" << std::endl;
    output_.format(*os_);
    *os_
        << std::endl
        << "Solution" << std::endl
        << "--------" << std::endl;
    output_.solution.format(*os_, parameters_.verbosity_level);
}

