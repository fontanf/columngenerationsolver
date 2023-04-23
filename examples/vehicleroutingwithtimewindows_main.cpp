#include "examples/vehicleroutingwithtimewindows.hpp"
#include "columngenerationsolver/read_args.hpp"

using namespace columngenerationsolver;
using namespace vehicleroutingwithtimewindows;

int main(int argc, char *argv[])
{
    MainArgs main_args;
    read_args(argc, argv, main_args);
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
    auto solution = run(main_args, p);

    // Write solution.
    std::string certificate_path = main_args.info.output->certificate_path;
    if (!certificate_path.empty()) {
        std::ofstream file(certificate_path);
        if (!file.good()) {
            throw std::runtime_error(
                    "Unable to open file \"" + certificate_path + "\".");
        }

        file << solution.size() << std::endl;
        for (auto colval: solution) {
            std::shared_ptr<ColumnExtra> extra
                = std::static_pointer_cast<ColumnExtra>(colval.first.extra);
            file << extra->route.size() << " ";
            for (LocationId location_id: extra->route)
                file << " " << location_id;
            file << std::endl;
        }
    }

    // Run checker.
    if (main_args.print_checker > 0
            && main_args.info.output->certificate_path != "") {
        os << std::endl
            << "Checker" << std::endl
            << "-------" << std::endl;
        instance.check(
                main_args.info.output->certificate_path,
                os,
                main_args.print_checker);
    }

    return 0;
}

