#include "examples/vehicleroutingwithtimewindows.hpp"
#include "columngenerationsolver/read_args.hpp"

using namespace columngenerationsolver;
using namespace vehicleroutingwithtimewindows;

int main(int argc, char *argv[])
{
    auto main_args = read_args(argc, argv);
    auto& os = main_args.info.os();

    // Create instance.
    Instance instance(main_args.instance_path, main_args.format);
    if (main_args.print_instance > 0) {
        os
            << "Instance" << std::endl
            << "--------" << std::endl;
        instance.print(os, main_args.print_instance);
        os << std::endl;
    }

    Parameters p = vehicleroutingwithtimewindows::get_parameters(instance);
    run(main_args, p);

    // Write solution.
    // TODO

    // Run checker.
    if (main_args.info.output->certificate_path != "") {
        os << std::endl;
        //instance.check(main_args.info.output->certificate_path); TODO
    }

    return 0;
}

