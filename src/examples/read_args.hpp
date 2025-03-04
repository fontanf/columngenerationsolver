#include "columngenerationsolver/algorithms/column_generation.hpp"
#include "columngenerationsolver/algorithms/greedy.hpp"
#include "columngenerationsolver/algorithms/limited_discrepancy_search.hpp"
#include "columngenerationsolver/algorithms/heuristic_tree_search.hpp"

#include <boost/program_options.hpp>

namespace columngenerationsolver
{

inline boost::program_options::options_description setup_args()
{
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("input,i", boost::program_options::value<std::string>()->required(), "set input path (required)")
        ("output,o", boost::program_options::value<std::string>()->default_value(""), "set JSON output path")
        ("certificate,c", boost::program_options::value<std::string>(), "set certificate path")
        ("format,f", boost::program_options::value<std::string>()->default_value(""), "set input file format (default: orlibrary)")
        ("algorithm,a", boost::program_options::value<std::string>()->default_value("iterative-beam-search"), "set algorithm")
        ("time-limit,t", boost::program_options::value<double>(), "set time limit in seconds\n  ex: 3600")
        ("verbosity-level,v", boost::program_options::value<int>(), "set verbosity level")
        ("only-write-at-the-end,e", "only write output and certificate files at the end")
        ("log,l", boost::program_options::value<std::string>(), "set log file")
        ("log-to-stderr", "write log to stderr")
        ("print-checker", boost::program_options::value<int>()->default_value(1), "print checker")

        ("linear-programming-solver", boost::program_options::value<SolverName>(), "set linear programming solver")
        ("internal-diving", boost::program_options::value<int>(), "set internal diving")
        ("discrepancy-limit", boost::program_options::value<int>(), "set discrepancy limit")
        ("automatic-stop", boost::program_options::value<bool>(), "set automatic stop")
        ;
    return desc;
}

using WriteSolutionFunction = std::function<void(const Solution&, const std::string&)>;

inline void read_args(
        Parameters& parameters,
        const WriteSolutionFunction& write_solution,
        const boost::program_options::variables_map& vm,
        const std::vector<std::shared_ptr<const Column>>& column_pool,
        const std::vector<std::shared_ptr<const Column>>& initial_columns)
{
    parameters.timer.set_sigint_handler();
    parameters.messages_to_stdout = true;
    if (vm.count("time-limit"))
        parameters.timer.set_time_limit(vm["time-limit"].as<double>());
    if (vm.count("verbosity-level"))
        parameters.verbosity_level = vm["verbosity-level"].as<int>();
    if (vm.count("log"))
        parameters.log_path = vm["log"].as<std::string>();
    parameters.log_to_stderr = vm.count("log-to-stderr");
    bool only_write_at_the_end = vm.count("only-write-at-the-end");
    if (!only_write_at_the_end) {
        parameters.new_solution_callback = [
            vm,
            write_solution](
                    const Output& output)
        {
            if (vm.count("output"))
                output.write_json_output(vm["output"].as<std::string>());
            if (vm.count("certificate")
                    && output.solution.feasible()) {
                write_solution(
                        output.solution,
                        vm["certificate"].as<std::string>());
            }
        };
    }
    parameters.initial_columns = initial_columns;
    parameters.column_pool = column_pool;
    if (vm.count("internal-diving"))
        parameters.internal_diving = vm["internal-diving"].as<int>();
}

inline void write_output(
        const WriteSolutionFunction& write_solution,
        const boost::program_options::variables_map& vm,
        const Output& output)
{
    if (vm.count("output"))
        output.write_json_output(vm["output"].as<std::string>());
    if (vm.count("certificate")
            && output.solution.feasible()) {
        write_solution(
                output.solution,
                vm["certificate"].as<std::string>());
    }
}

inline const Output run_column_generation(
        const Model& model,
        const WriteSolutionFunction& write_solution,
        const boost::program_options::variables_map& vm,
        const std::vector<std::shared_ptr<const Column>>& column_pool,
        const std::vector<std::shared_ptr<const Column>>& initial_columns)
{
    ColumnGenerationParameters parameters;
    read_args(parameters, write_solution, vm, column_pool, initial_columns);
    if (vm.count("linear-programming-solver"))
        parameters.solver_name = vm["linear-programming-solver"].as<SolverName>();
    const Output output = column_generation(model, parameters);
    write_output(write_solution, vm, output);
    return output;
}

inline const Output run_greedy(
        const Model& model,
        const WriteSolutionFunction& write_solution,
        const boost::program_options::variables_map& vm,
        const std::vector<std::shared_ptr<const Column>>& column_pool,
        const std::vector<std::shared_ptr<const Column>>& initial_columns)
{
    GreedyParameters parameters;
    read_args(parameters, write_solution, vm, column_pool, initial_columns);
    if (vm.count("linear-programming-solver")) {
        parameters.column_generation_parameters.solver_name
            = vm["linear-programming-solver"].as<SolverName>();
    }
    const Output output = greedy(model, parameters);
    write_output(write_solution, vm, output);
    return output;
}

inline const Output run_limited_discrepancy_search(
        const Model& model,
        const WriteSolutionFunction& write_solution,
        const boost::program_options::variables_map& vm,
        const std::vector<std::shared_ptr<const Column>>& column_pool,
        const std::vector<std::shared_ptr<const Column>>& initial_columns)
{
    LimitedDiscrepancySearchParameters parameters;
    read_args(parameters, write_solution, vm, column_pool, initial_columns);
    if (vm.count("linear-programming-solver")) {
        parameters.column_generation_parameters.solver_name
            = vm["linear-programming-solver"].as<SolverName>();
    }
    if (vm.count("discrepancy-limit"))
        parameters.discrepancy_limit = vm["discrepancy-limit"].as<int>();
    if (vm.count("automatic-stop"))
        parameters.automatic_stop = vm["automatic-stop"].as<bool>();
    const Output output = limited_discrepancy_search(model, parameters);
    write_output(write_solution, vm, output);
    return output;
}

inline const Output run_heuristic_tree_search(
        const Model& model,
        const WriteSolutionFunction& write_solution,
        const boost::program_options::variables_map& vm,
        const std::vector<std::shared_ptr<const Column>>& column_pool,
        const std::vector<std::shared_ptr<const Column>>& initial_columns)
{
    HeuristicTreeSearchParameters parameters;
    read_args(parameters, write_solution, vm, column_pool, initial_columns);
    if (vm.count("linear-programming-solver")) {
        parameters.column_generation_parameters.solver_name
            = vm["linear-programming-solver"].as<SolverName>();
    }
    const Output output = heuristic_tree_search(model, parameters);
    write_output(write_solution, vm, output);
    return output;
}

inline Output run(
        const Model& model,
        const WriteSolutionFunction& write_solution,
        const boost::program_options::variables_map& vm,
        const std::vector<std::shared_ptr<const Column>>& column_pool = {},
        const std::vector<std::shared_ptr<const Column>>& initial_columns = {})
{

#if XPRESS_FOUND
    if (column_generation_parameters.linear_programming_solver
            == LinearProgrammingSolver::Xpress)
        XPRSinit(NULL);
#endif

    std::string algorithm = vm["algorithm"].as<std::string>();
    if (algorithm == "column-generation") {
        return run_column_generation(model, write_solution, vm, column_pool, initial_columns);
    } else if (algorithm == "greedy") {
        return run_greedy(model, write_solution, vm, column_pool, initial_columns);
    } else if (algorithm == "limited-discrepancy-search") {
        return run_limited_discrepancy_search(model, write_solution, vm, column_pool, initial_columns);
    } else if (algorithm == "heuristic-tree-search") {
        return run_heuristic_tree_search(model, write_solution, vm, column_pool, initial_columns);
    } else {
        throw std::invalid_argument(
                "Unknown algorithm \"" + algorithm + "\".");
    }

#if XPRESS_FOUND
    if (column_generation_parameters.linear_programming_solver
            == LinearProgrammingSolver::Xpress)
        XPRSfree();
#endif

    return Output(model);
}

}
