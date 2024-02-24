#include "examples/cutting_stock.hpp"
#include "columngenerationsolver/read_args.hpp"

using namespace columngenerationsolver;
using namespace cutting_stock;

int main(int argc, char *argv[])
{
    // Setup options.
    boost::program_options::options_description desc = setup_args();
    desc.add_options()
        //("guide,g", boost::program_options::value<GuideId>(), "")
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

    // Create instance.
    InstanceBuilder instance_builder;
    instance_builder.read(
            vm["input"].as<std::string>(),
            vm["format"].as<std::string>());
    const Instance instance = instance_builder.build();

    // Create model.
    Model model = get_model(instance);

    // Solve.
    auto output = run(model, write_solution, vm);

    // Run checker.
    if (vm.count("certificate")
            && vm["print-checker"].as<int>() > 0) {
        std::cout << std::endl
            << "Checker" << std::endl
            << "-------" << std::endl;
        instance.check(
                vm["certificate"].as<std::string>(),
                std::cout,
                vm["print-checker"].as<int>());
    }

    return 0;
}
