#include "columngenerationsolver/columngenerationsolver.hpp"

#include <boost/program_options.hpp>

namespace columngenerationsolver
{

inline ColumnGenerationOptionalParameters read_columngeneration_args(
        std::string args_string)
{
    std::vector<std::string> args
        = boost::program_options::split_unix(args_string);
    std::vector<char*> argv;
    std::string dummy = "dummy";
    argv.push_back(const_cast<char*>(dummy.c_str()));
    for (Counter i = 0; i < (Counter)args.size(); ++i)
        argv.push_back(const_cast<char*>(args[i].c_str()));

    ColumnGenerationOptionalParameters parameters;
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("iteration-limit,i", boost::program_options::value<Counter>(&parameters.iteration_limit), "")
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

inline LimitedDiscrepancySearchOptionalParameters read_limiteddiscrepancysearch_args(
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

}
