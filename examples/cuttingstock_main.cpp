#include "examples/cuttingstock.hpp"
#include "columngenerationsolver/read_args.hpp"

using namespace columngenerationsolver;
using namespace cuttingstock;

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

    Parameters p = cuttingstock::get_parameters(instance);
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
            const Column& column = colval.first;
            Value value = colval.second;
            file << std::round(value)
                << " " << column.row_indices.size() << "  ";
            for (RowIdx pos = 0; pos < (RowIdx)column.row_indices.size(); ++pos) {
                file << "  " << column.row_indices[pos]
                    << " " << std::round(column.row_coefficients[pos]);
            }
            file << std::endl;
        }
    }

    // Run checker.
    if (main_args.print_checker > 0
            && certificate_path != "") {
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

