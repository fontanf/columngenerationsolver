#include "examples/cuttingstock.hpp"
#include "examples/multipleknapsack.hpp"
#include "examples/capacitatedvehiclerouting.hpp"
#include "examples/starobservationscheduling.hpp"

#include "columngenerationsolver/read_args.hpp"

#include <boost/program_options.hpp>

using namespace columngenerationsolver;

namespace po = boost::program_options;

void run(
        std::string algorithm,
        std::string columngeneration_args_string,
        bool verbose,
        double time_limit,
        Parameters& p)
{
    optimizationtools::Info info = optimizationtools::Info()
        .set_verbose(verbose)
        .set_timelimit(time_limit)
        ;

    std::vector<std::string> algorithm_args
        = boost::program_options::split_unix(algorithm);
    std::vector<char*> algorithm_argv;
    for (Counter i = 0; i < (Counter)algorithm_args.size(); ++i)
        algorithm_argv.push_back(const_cast<char*>(algorithm_args[i].c_str()));

    ColumnGenerationOptionalParameters columngeneration_parameters
        = read_columngeneration_args(columngeneration_args_string);

    if (algorithm_args[0] == "columngeneration") {
        columngeneration_parameters.info = info;
        columngeneration(p, columngeneration_parameters);
    } else if (algorithm_args[0] == "greedy") {
        GreedyOptionalParameters op;
        op.info = info;
        op.columngeneration_parameters = columngeneration_parameters;
        greedy(p, op);
    } else if (algorithm_args[0] == "limiteddiscrepancysearch") {
        auto op = read_limiteddiscrepancysearch_args(algorithm_argv);
        op.info = info;
        op.columngeneration_parameters = columngeneration_parameters;
        limiteddiscrepancysearch(p, op);
    } else if (algorithm_args[0] == "heuristictreesearch") {
        auto op = read_heuristictreesearch_args(algorithm_argv);
        op.info = info;
        op.columngeneration_parameters = columngeneration_parameters;
        heuristictreesearch(p, op);
    } else {
        std::cerr << "\033[31m" << "ERROR, unknown algorithm: '" << algorithm_args[0] << "'.\033[0m" << std::endl;
    }
}

int main(int argc, char *argv[])
{

    // Parse program options

    std::string problem = "cuttingstock";
    std::string instance_path = "";
    std::string format = "";
    std::string algorithm = "limiteddiscrepancysearch";
    std::string columngeneration_args_string = "";
    LinearProgrammingSolver linear_programming_solver = LinearProgrammingSolver::CPLEX;
    double time_limit = std::numeric_limits<double>::infinity();

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("problem,p", po::value<std::string>(&problem)->required(), "set problem (required)")
        ("input,i", po::value<std::string>(&instance_path)->required(), "set input file (required)")
        ("format,f", po::value<std::string>(&format), "set input file format (default: orlibrary)")
        ("algorithm,a", po::value<std::string>(&algorithm), "set algorithm")
        ("linear-programming-solver,s", po::value<LinearProgrammingSolver>(&linear_programming_solver), "set linear programming solver")
        ("column-generation-parameters,c", po::value<std::string>(&columngeneration_args_string), "set column generation parameters")
        ("time-limit,t", po::value<double>(&time_limit), "Time limit in seconds\n  ex: 3600")
        ("verbose,v", "")
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

    // Run algorithm

    if (problem == "cuttingstock") {
        cuttingstocksolver::Instance instance(instance_path, format);
        Parameters p = cuttingstocksolver::get_parameters(instance, linear_programming_solver);
        run(algorithm, columngeneration_args_string, vm.count("verbose"), time_limit, p);
    } else if (problem == "multipleknapsack") {
        multipleknapsacksolver::Instance instance(instance_path, format);
        Parameters p = multipleknapsacksolver::get_parameters(instance, linear_programming_solver);
        run(algorithm, columngeneration_args_string, vm.count("verbose"), time_limit, p);
    } else if (problem == "capacitatedvehiclerouting") {
        capacitatedvehicleroutingsolver::Instance instance(instance_path, format);
        Parameters p = capacitatedvehicleroutingsolver::get_parameters(instance, linear_programming_solver);
        run(algorithm, columngeneration_args_string, vm.count("verbose"), time_limit, p);
    } else if (problem == "starobservationscheduling") {
        starobservationschedulingsolver::Instance instance(instance_path, format);
        Parameters p = starobservationschedulingsolver::get_parameters(instance, linear_programming_solver);
        run(algorithm, columngeneration_args_string, vm.count("verbose"), time_limit, p);
    } else {
        std::cerr << "\033[31m" << "ERROR, unknown problem: '" << problem << "'.\033[0m" << std::endl;
        return 1;
    }

    return 0;
}

