#include "examples/cuttingstock.hpp"
#include "examples/multipleknapsack.hpp"
#include "examples/binpackingwithconflicts.hpp"
#include "examples/capacitatedvehiclerouting.hpp"
#include "examples/vehicleroutingwithtimewindows.hpp"
#include "examples/capacitatedopenvehiclerouting.hpp"
#include "examples/parallelschedulingwithfamilysetuptimestwct.hpp"
#include "examples/starobservationscheduling.hpp"

#include "columngenerationsolver/read_args.hpp"

#include <boost/program_options.hpp>

using namespace columngenerationsolver;

namespace po = boost::program_options;

void run(
        std::string algorithm,
        std::string column_generation_args_string,
        const optimizationtools::Info& info,
        Parameters& p)
{
    std::vector<std::string> algorithm_args
        = boost::program_options::split_unix(algorithm);
    std::vector<char*> algorithm_argv;
    for (Counter i = 0; i < (Counter)algorithm_args.size(); ++i)
        algorithm_argv.push_back(const_cast<char*>(algorithm_args[i].c_str()));

    ColumnGenerationOptionalParameters column_generation_parameters
        = read_column_generation_args(column_generation_args_string);

    if (algorithm_args[0] == "column_generation") {
        column_generation_parameters.info = info;
        column_generation(p, column_generation_parameters);
    } else if (algorithm_args[0] == "greedy") {
        GreedyOptionalParameters op;
        op.info = info;
        op.column_generation_parameters = column_generation_parameters;
        greedy(p, op);
    } else if (algorithm_args[0] == "limited_discrepancy_search") {
        auto op = read_limited_discrepancy_search_args(algorithm_argv);
        op.info = info;
        op.column_generation_parameters = column_generation_parameters;
        limited_discrepancy_search(p, op);
    } else if (algorithm_args[0] == "heuristic_tree_search") {
        auto op = read_heuristic_tree_search_args(algorithm_argv);
        op.info = info;
        op.column_generation_parameters = column_generation_parameters;
        heuristic_tree_search(p, op);
    } else {
        std::cerr << "\033[31m" << "ERROR, unknown algorithm: '" << algorithm_args[0] << "'.\033[0m" << std::endl;
    }
}

int main(int argc, char *argv[])
{

    // Parse program options

    std::string problem = "cuttingstock";
    std::string instance_path = "";
    std::string output_path = "";
    std::string certificate_path = "";
    std::string format = "";
    std::string algorithm = "heuristictreesearch";
    std::string column_generation_args_string = "";
    double time_limit = std::numeric_limits<double>::infinity();

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("problem,p", po::value<std::string>(&problem)->required(), "set problem (required)")
        ("input,i", po::value<std::string>(&instance_path)->required(), "set input path (required)")
        ("output,o", po::value<std::string>(&output_path), "set JSON output path")
        ("certificate,c", po::value<std::string>(&certificate_path), "set certificate path")
        ("format,f", po::value<std::string>(&format), "set input file format (default: orlibrary)")
        ("algorithm,a", po::value<std::string>(&algorithm), "set algorithm")
        ("column-generation-parameters,g", po::value<std::string>(&column_generation_args_string), "set column generation parameters")
        ("time-limit,t", po::value<double>(&time_limit), "Time limit in seconds\n  ex: 3600")
        ("verbose,v", "")
        ("print-instance", "")
        ("print-solution", "")
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;;
        return 1;
    }
    try {
        po::notify(vm);
    } catch (const po::required_option& e) {
        std::cout << desc << std::endl;;
        return 1;
    }

    optimizationtools::Info info = optimizationtools::Info()
        .set_verbose(vm.count("verbose"))
        .set_time_limit(time_limit)
        .set_json_output_path(output_path)
        .set_sigint_handler()
        ;

    // Run algorithm

    if (problem == "cuttingstock") {
        cuttingstock::Instance instance(instance_path, format);
        if (vm.count("print-instance"))
            std::cout << instance << std::endl;
        Parameters p = cuttingstock::get_parameters(instance);
        run(algorithm, column_generation_args_string, info, p);

    } else if (problem == "multipleknapsack") {
        multipleknapsack::Instance instance(instance_path, format);
        if (vm.count("print-instance"))
            std::cout << instance << std::endl;
        Parameters p = multipleknapsack::get_parameters(instance);
        run(algorithm, column_generation_args_string, info, p);

    } else if (problem == "binpackingwithconflicts") {
        binpackingwithconflicts::Instance instance(instance_path, format);
        //if (vm.count("print-instance")) TODO
        //    std::cout << instance << std::endl;
        Parameters p = binpackingwithconflicts::get_parameters(instance);
        run(algorithm, column_generation_args_string, info, p);

    } else if (problem == "capacitatedvehiclerouting") {
        capacitatedvehiclerouting::Instance instance(instance_path, format);
        //if (vm.count("print-instance")) TODO
        //    std::cout << instance << std::endl;
        Parameters p = capacitatedvehiclerouting::get_parameters(instance);
        run(algorithm, column_generation_args_string, info, p);

    } else if (problem == "vehicleroutingwithtimewindows") {
        vehicleroutingwithtimewindows::Instance instance(instance_path, format);
        //if (vm.count("print-instance")) TODO
        //    std::cout << instance << std::endl;
        Parameters p = vehicleroutingwithtimewindows::get_parameters(instance);
        run(algorithm, column_generation_args_string, info, p);

    } else if (problem == "capacitatedopenvehiclerouting") {
        capacitatedopenvehiclerouting::Instance instance(instance_path, format);
        if (vm.count("print-instance"))
            std::cout << instance << std::endl;
        Parameters p = capacitatedopenvehiclerouting::get_parameters(instance);
        run(algorithm, column_generation_args_string, info, p);

    } else if (problem == "parallelschedulingwithfamilysetuptimestwct") {
        parallelschedulingwithfamilysetuptimestwct::Instance instance(instance_path, format);
        if (vm.count("print-instance"))
            std::cout << instance << std::endl;
        Parameters p = parallelschedulingwithfamilysetuptimestwct::get_parameters(instance);
        run(algorithm, column_generation_args_string, info, p);

    } else if (problem == "starobservationscheduling") {
        starobservationscheduling::Instance instance(instance_path, format);
        //if (vm.count("print-instance")) TODO
        //    std::cout << instance << std::endl;
        Parameters p = starobservationscheduling::get_parameters(instance);
        run(algorithm, column_generation_args_string, info, p);

    } else {
        std::cerr << "\033[31m" << "ERROR, unknown problem: '" << problem << "'.\033[0m" << std::endl;
        return 1;
    }

    return 0;
}

