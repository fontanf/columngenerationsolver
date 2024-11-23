/**
 * Capacitated vehicle routing problem.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/capacitated_vehicle_routing.hpp
 *
 * The linear programming formulation of the problem based on Dantzig–Wolfe
 * decomposition is written as follows:
 *
 * Variables:
 * - yᵏ ∈ {0, 1} representing a feasible route.
 *   yᵏ = 1 iff the corresponding route is selected.
 *   dᵏ the length of route yᵏ.
 *   xⱼᵏ = 1 iff customer j is visited in route yᵏ.
 *
 * Program:
 *
 * min ∑ₖ dᵏ yᵏ
 *
 * 1 <= ∑ₖ xⱼᵏ yᵏ <= 1     for all customers j
 *                                    (each customer is visited exactly once)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of negative reduced cost.
 * The reduced cost of a variable yᵏ is given by:
 * rc(yᵏ) = dᵏ - ∑ⱼ xⱼᵏ vⱼ
 *
 * Therefore, finding a variable of minimum reduced cost reduces to solving
 * an Elementary Shortest Path Problems with Resource Constraints.
 *
 */

#pragma once

#include "columngenerationsolver/commons.hpp"

#include "columngenerationsolver/examples/pricingsolver/espprc.hpp"

#include "orproblems/routing/capacitated_vehicle_routing.hpp"

#include "treesearchsolver/iterative_beam_search.hpp"

#include "optimizationtools/utils/utils.hpp"

namespace columngenerationsolver
{
namespace capacitated_vehicle_routing
{

using namespace orproblems::capacitated_vehicle_routing;

template <typename Distances>
class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(
            const Instance& instance,
            const Distances& distances):
        instance_(instance),
        distances_(distances),
        visited_customers_(instance.number_of_locations(), 0)
    { }

    virtual inline std::vector<std::shared_ptr<const Column>> initialize_pricing(
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns);

    virtual inline PricingOutput solve_pricing(
            const std::vector<Value>& duals);

    void set_beam_search_size_of_the_queue(treesearchsolver::NodeId bs_size_of_the_queue) { bs_size_of_the_queue_ = bs_size_of_the_queue; }

private:

    /** Instance. */
    const Instance& instance_;

    /** Distances. */
    const Distances& distances_;

    std::vector<Demand> visited_customers_;

    std::vector<LocationId> espp2vrp_;

    treesearchsolver::NodeId bs_size_of_the_queue_ = 128;

};

template <typename Distances>
inline columngenerationsolver::Model get_model(
        const Distances& distances,
        const Instance& instance)
{
    columngenerationsolver::Model model;

    model.objective_sense = optimizationtools::ObjectiveDirection::Minimize;

    // Rows.
    for (LocationId location_id = 1;
            location_id < instance.number_of_locations();
            ++location_id) {
        Row row;
        row.lower_bound = 1;
        row.upper_bound = 1;
        row.coefficient_lower_bound = 0;
        row.coefficient_upper_bound = 1;
        model.rows.push_back(row);
    }

    // Pricing solver.
    model.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver<Distances>(instance, distances));

    return model;
}

template <typename Distances>
std::vector<std::shared_ptr<const Column>> PricingSolver<Distances>::initialize_pricing(
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns)
{
    std::fill(visited_customers_.begin(), visited_customers_.end(), 0);
    for (const auto& p: fixed_columns) {
        const Column& column = *(p.first);
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (const LinearTerm& element: column.elements) {
            if (element.coefficient < 0.5)
                continue;
            // row_index + 1 since there is not constraint for location 0 which
            // is the depot.
            visited_customers_[element.row + 1] = 1;
        }
    }
    return {};
}

struct ColumnExtra
{
    std::vector<LocationId> route;
};

template <typename Distances>
typename PricingSolver<Distances>::PricingOutput PricingSolver<Distances>::solve_pricing(
            const std::vector<Value>& duals)
{
    PricingOutput output;

    // Build subproblem instance.
    espp2vrp_.clear();
    espp2vrp_.push_back(0);
    for (LocationId location_id = 1;
            location_id < instance_.number_of_locations();
            ++location_id) {
        if (visited_customers_[location_id] == 1)
            continue;
        espp2vrp_.push_back(location_id);
    }

    LocationId espp_number_of_locations = espp2vrp_.size();
    if (espp_number_of_locations == 1)
        return output;
    espprc::InstanceBuilder espp_instance_builder(espp_number_of_locations);
    for (LocationId espp_location_id = 0;
            espp_location_id < espp_number_of_locations;
            ++espp_location_id) {
        LocationId location_id = espp2vrp_[espp_location_id];
        espp_instance_builder.set_demand(
                espp_location_id,
                instance_.demand(location_id));
        espp_instance_builder.set_profit(
                espp_location_id,
                ((location_id != 0)? duals[location_id - 1]: 0));
        for (LocationId espp_location_id_2 = 0;
                espp_location_id_2 < espp_number_of_locations;
                ++espp_location_id_2) {
            if (espp_location_id_2 == espp_location_id)
                continue;
            LocationId location_id_2 = espp2vrp_[espp_location_id_2];
            espp_instance_builder.set_distance(
                    espp_location_id,
                    espp_location_id_2,
                    distances_.distance(location_id, location_id_2));
        }
    }
    espprc::Instance espp_instance = espp_instance_builder.build();

    // Solve subproblem instance.
    espprc::BranchingScheme branching_scheme(espp_instance);
    treesearchsolver::IterativeBeamSearchParameters<espprc::BranchingScheme> espp_parameters;
    espp_parameters.maximum_size_of_the_solution_pool = 1;
    espp_parameters.minimum_size_of_the_queue = bs_size_of_the_queue_;
    espp_parameters.maximum_size_of_the_queue = bs_size_of_the_queue_;
    espp_parameters.verbosity_level = 0;
    auto espp_output = treesearchsolver::iterative_beam_search(
            branching_scheme, espp_parameters);

    // Retrieve column.
    for (const std::shared_ptr<espprc::BranchingScheme::Node>& node:
            espp_output.solution_pool.solutions()) {
        if (node->last_location_id == 0)
            continue;
        std::vector<LocationId> solution; // Without the depot.
        for (auto node_tmp = node;
                node_tmp->parent != nullptr;
                node_tmp = node_tmp->parent) {
            solution.push_back(espp2vrp_[node_tmp->last_location_id]);
        }
        std::reverse(solution.begin(), solution.end());

        Column column;
        LocationId location_id_prev = 0;
        for (LocationId location_id: solution) {
            LinearTerm element;
            element.row = location_id - 1;
            element.coefficient = 1;
            column.elements.push_back(element);
            column.objective_coefficient += distances_.distance(location_id_prev, location_id);
            location_id_prev = location_id;
        }
        column.objective_coefficient += distances_.distance(location_id_prev, 0);
        ColumnExtra extra {solution};
        column.extra = std::shared_ptr<void>(new ColumnExtra(extra));
        output.columns.push_back(std::shared_ptr<const Column>(new Column(column)));
    }

    return output;
}

inline void write_solution(
        const Solution& solution,
        const std::string& certificate_path)
{
    std::ofstream file(certificate_path);
    if (!file.good()) {
        throw std::runtime_error(
                "Unable to open file \"" + certificate_path + "\".");
    }

    file << solution.columns().size() << std::endl;
    for (auto colval: solution.columns()) {
        std::shared_ptr<ColumnExtra> extra
            = std::static_pointer_cast<ColumnExtra>(colval.first->extra);
        file << extra->route.size() << " ";
        for (LocationId location_id: extra->route)
            file << " " << location_id;
        file << std::endl;
    }
}

}
}
