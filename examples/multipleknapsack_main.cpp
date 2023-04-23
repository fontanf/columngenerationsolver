#include "examples/multipleknapsack.hpp"
#include "columngenerationsolver/read_args.hpp"

using namespace columngenerationsolver;
using namespace multipleknapsack;

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

    Parameters p = multipleknapsack::get_parameters(instance);
    auto solution = run(main_args, p);

    // Write solution.
    std::string certificate_path = main_args.info.output->certificate_path;
    if (!certificate_path.empty()) {
        std::ofstream file(certificate_path);
        if (!file.good()) {
            throw std::runtime_error(
                    "Unable to open file \"" + certificate_path + "\".");
        }

        std::vector<std::vector<ItemId>> sol(instance.number_of_knapsacks());
        for (auto colval: solution) {
            const Column& column = colval.first;
            //Value value = colval.second;
            // Get the knapsack id.
            KnapsackId knapsack_id = -1;
            for (RowIdx pos = 0; pos < (RowIdx)column.row_indices.size(); ++pos) {
                if (column.row_coefficients[pos] > 0.5
                        && column.row_indices[pos] < instance.number_of_knapsacks()) {
                    knapsack_id = column.row_indices[pos];
                }
            }
            // Get the items.
            for (RowIdx pos = 0; pos < (RowIdx)column.row_indices.size(); ++pos) {
                if (column.row_coefficients[pos] > 0.5
                        && column.row_indices[pos] >= instance.number_of_knapsacks()) {
                    ItemId item_id = column.row_indices[pos] - instance.number_of_knapsacks();
                    sol[knapsack_id].push_back(item_id);
                }
            }
        }
        // Write.
        for (KnapsackId knapsack_id = 0;
                knapsack_id < instance.number_of_knapsacks();
                ++knapsack_id) {
            file << sol[knapsack_id].size() << std::endl;
            for (ItemId item_id: sol[knapsack_id])
                file << " " << item_id;
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

