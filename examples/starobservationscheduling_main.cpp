#include "examples/starobservationscheduling.hpp"
#include "columngenerationsolver/read_args.hpp"

using namespace columngenerationsolver;
using namespace starobservationscheduling;

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

    Parameters p = starobservationscheduling::get_parameters(instance);
    auto solution = run(main_args, p);

    // Write solution.
    std::string certificate_path = main_args.info.output->certificate_path;
    if (!certificate_path.empty()) {
        std::ofstream file(certificate_path);
        if (!file.good()) {
            throw std::runtime_error(
                    "Unable to open file \"" + certificate_path + "\".");
        }

        std::vector<std::vector<std::pair<ObservableId, Time>>> sol(instance.number_of_nights());
        for (const auto& colval: solution) {
            std::shared_ptr<ColumnExtra> extra
                = std::static_pointer_cast<ColumnExtra>(colval.first.extra);
            for (singlenightstarobservationschedulingsolver::TargetId snsosp_observation_pos = 0;
                    snsosp_observation_pos < extra->snsosp_solution.number_of_observations();
                    ++snsosp_observation_pos) {
                const auto& snsosp_observation = extra->snsosp_solution.observation(snsosp_observation_pos);
                ObservableId observable_id = extra->snsosp2sosp[snsosp_observation.target_id];
                sol[extra->night_id].push_back({observable_id, snsosp_observation.start_time});
            }
        }
        // Write.
        for (NightId night_id = 0;
                night_id < instance.number_of_nights();
                ++night_id) {
            sort(sol[night_id].begin(), sol[night_id].end(),
                    [](
                        const std::pair<ObservableId, Time>& os1,
                        const std::pair<ObservableId, Time>& os2) -> bool
                    {
                        return os1.second < os2.second;
                    });
            file << sol[night_id].size() << std::endl;
            for (const auto& os: sol[night_id])
                file << " " << os.first;
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

