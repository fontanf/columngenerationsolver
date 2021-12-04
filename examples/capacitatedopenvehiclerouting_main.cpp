#include "examples/capacitatedopenvehiclerouting.hpp"
#include "columngenerationsolver/read_args.hpp"

using namespace columngenerationsolver;
using namespace capacitatedopenvehiclerouting;

int main(int argc, char *argv[])
{
    auto main_args = read_args(argc, argv);

    // Create instance.
    Instance instance(main_args.instance_path, main_args.format);
    if (main_args.print_instance)
        std::cout << instance << std::endl;

    Parameters p = capacitatedopenvehiclerouting::get_parameters(instance);
    run(main_args, p);

    // Write solution.
    // TODO

    // Run checker.
    if (main_args.info.output->certificate_path != "") {
        std::cout << std::endl;
        //instance.check(main_args.info.output->certificate_path); TODO
    }

    return 0;
}

