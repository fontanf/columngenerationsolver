#include "columngenerationsolver/algorithms/column_generation.hpp"
#include "columngenerationsolver/algorithms/greedy.hpp"
#include "columngenerationsolver/algorithms/limited_discrepancy_search.hpp"
#include "columngenerationsolver/algorithms/heuristic_tree_search.hpp"

#include <boost/program_options.hpp>

namespace columngenerationsolver
{

inline ColumnGenerationOptionalParameters read_column_generation_args(
        const std::vector<char*> argv)
{
    ColumnGenerationOptionalParameters parameters;
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("linear-programming-solver,s", boost::program_options::value<LinearProgrammingSolver>(&parameters.linear_programming_solver), "set linear programming solver")
        ("maximum-number-of-iterations,i", boost::program_options::value<Counter>(&parameters.maximum_number_of_iterations), "")
        ("static-wentges-smoothing-parameter,a", boost::program_options::value<Value>(&parameters.static_wentges_smoothing_parameter), "")
        ("static-directional-smoothing-parameter,b", boost::program_options::value<Value>(&parameters.static_directional_smoothing_parameter), "")
        ("self-adjusting-wentges-smoothing,A", boost::program_options::value<bool>(&parameters.self_adjusting_wentges_smoothing), "")
        ("automatic-directional-smoothing,B", boost::program_options::value<bool>(&parameters.automatic_directional_smoothing), "")
        ;
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line((Counter)argv.size(), argv.data(), desc), vm);
    try {
        boost::program_options::notify(vm);
    } catch (const boost::program_options::required_option& e) {
        std::cout << desc << std::endl;;
        throw "";
    }
    return parameters;
}

inline LimitedDiscrepancySearchOptionalParameters read_limited_discrepancy_search_args(
        const std::vector<char*> argv)
{
    LimitedDiscrepancySearchOptionalParameters parameters;
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("discrepancy-limit,d", boost::program_options::value<Value>(&parameters.discrepancy_limit), "")
        ;
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line((Counter)argv.size(), argv.data(), desc), vm);
    try {
        boost::program_options::notify(vm);
    } catch (const boost::program_options::required_option& e) {
        std::cout << desc << std::endl;;
        throw "";
    }
    return parameters;
}

inline HeuristicTreeSearchOptionalParameters read_heuristic_tree_search_args(
        const std::vector<char*> argv)
{
    HeuristicTreeSearchOptionalParameters parameters;
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("growth-rate,g", boost::program_options::value<double>(&parameters.growth_rate), "")
        ;
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line((Counter)argv.size(), argv.data(), desc), vm);
    try {
        boost::program_options::notify(vm);
    } catch (const boost::program_options::required_option& e) {
        std::cout << desc << std::endl;;
        throw "";
    }
    return parameters;
}

struct MainArgs
{
    std::string instance_path = "";
    std::string format = "";
    std::vector<std::string> algorithm_args;
    std::vector<char*> algorithm_argv;
    std::vector<std::string> column_generation_args;
    std::vector<char*> column_generation_argv;
    optimizationtools::Info info = optimizationtools::Info();
    int print_instance = 1;
    int print_solution = 1;
    int print_checker = 1;
};

MainArgs read_args(
        int argc,
        char *argv[],
        MainArgs& main_args)
{
    std::string output_path = "";
    std::string certificate_path = "";
    std::string algorithm = "limited-discrepancy-search";
    std::string column_generation_parameters = "";
    double time_limit = std::numeric_limits<double>::infinity();
    int verbosity_level = 1;

    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("input,i", boost::program_options::value<std::string>(&main_args.instance_path)->required(), "set input path (required)")
        ("output,o", boost::program_options::value<std::string>(&output_path), "set JSON output path")
        ("certificate,c", boost::program_options::value<std::string>(&certificate_path), "set certificate path")
        ("format,f", boost::program_options::value<std::string>(&main_args.format), "set input file format (default: orlibrary)")
        ("algorithm,a", boost::program_options::value<std::string>(&algorithm), "set algorithm")
        ("column-generation-parameters,g", boost::program_options::value<std::string>(&column_generation_parameters), "set column generation parameters")
        ("time-limit,t", boost::program_options::value<double>(&time_limit), "Time limit in seconds\n  ex: 3600")
        ("only-write-at-the-end,e", "Only write output and certificate files at the end")
        ("verbosity-level,v", boost::program_options::value<int>(&verbosity_level), "")
        ("print-instance", boost::program_options::value<int>(&main_args.print_instance), "print instance")
        ("print-solution", boost::program_options::value<int>(&main_args.print_solution), "print solution")
        ("print-checker", boost::program_options::value<int>(&main_args.print_checker), "print checker")
        ;
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;;
        throw "";
    }
    try {
        boost::program_options::notify(vm);
    } catch (const boost::program_options::required_option& e) {
        std::cout << desc << std::endl;;
        throw "";
    }

    main_args.algorithm_args = boost::program_options::split_unix(algorithm);
    for (std::string& s: main_args.algorithm_args)
        main_args.algorithm_argv.push_back(const_cast<char*>(s.c_str()));

    main_args.column_generation_args = boost::program_options::split_unix(column_generation_parameters);
    std::string dummy = "dummy";
    main_args.column_generation_argv.push_back(const_cast<char*>(dummy.c_str()));
    for (std::string& s: main_args.column_generation_args)
        main_args.column_generation_argv.push_back(const_cast<char*>(s.c_str()));

    main_args.info = optimizationtools::Info()
        .set_verbosity_level(verbosity_level)
        .set_time_limit(time_limit)
        .set_certificate_path(certificate_path)
        .set_json_output_path(output_path)
        .set_only_write_at_the_end(vm.count("only-write-at-the-end"))
        .set_only_write_at_the_end(true)
        .set_sigint_handler()
        ;

    return main_args;
}

std::vector<std::pair<Column, Value>> run(
        const MainArgs& main_args,
        Parameters& p)
{
    ColumnGenerationOptionalParameters column_generation_parameters
        = read_column_generation_args(main_args.column_generation_argv);
    std::vector<std::pair<Column, Value>> solution;

#if XPRESS_FOUND
    if (column_generation_parameters.linear_programming_solver
            == LinearProgrammingSolver::Xpress)
        XPRSinit(NULL);
#endif

    if (strcmp(main_args.algorithm_argv[0], "column-generation") == 0) {
        column_generation_parameters.info = main_args.info;
        auto column_generation_output = column_generation(p, column_generation_parameters);
    } else if (strcmp(main_args.algorithm_argv[0], "greedy") == 0) {
        GreedyOptionalParameters op;
        op.info = main_args.info;
        op.column_generation_parameters = column_generation_parameters;
        auto greedy_output = greedy(p, op);
        solution = greedy_output.solution;
    } else if (strcmp(main_args.algorithm_argv[0], "limited-discrepancy-search") == 0) {
        auto op = read_limited_discrepancy_search_args(main_args.algorithm_argv);
        op.info = main_args.info;
        op.column_generation_parameters = column_generation_parameters;
        auto limited_discrepancy_search_output = limited_discrepancy_search(p, op);
        solution = limited_discrepancy_search_output.solution;
    } else if (strcmp(main_args.algorithm_argv[0], "heuristic-tree-search") == 0) {
        auto op = read_heuristic_tree_search_args(main_args.algorithm_argv);
        op.info = main_args.info;
        op.column_generation_parameters = column_generation_parameters;
        auto heuristic_tree_search_output = heuristic_tree_search(p, op);
        solution = heuristic_tree_search_output.solution;
    } else {
        std::cerr << "\033[31m" << "ERROR, unknown algorithm: '" << main_args.algorithm_argv[0] << "'.\033[0m" << std::endl;
    }

#if XPRESS_FOUND
    if (column_generation_parameters.linear_programming_solver
            == LinearProgrammingSolver::Xpress)
        XPRSfree();
#endif

    return solution;
}
}
