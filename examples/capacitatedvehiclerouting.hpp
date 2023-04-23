/**
 * Capacitated Vehicle Routing Problem.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/capacitatedvehiclerouting.hpp
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

#include "examples/pricingsolver/espprc.hpp"

#include "orproblems/capacitatedvehiclerouting.hpp"

#include "treesearchsolver/iterative_beam_search.hpp"
#include "treesearchsolver/best_first_search.hpp"
#include "treesearchsolver/iterative_memory_bounded_best_first_search.hpp"

#include "optimizationtools/utils/utils.hpp"

namespace columngenerationsolver
{

namespace capacitatedvehiclerouting
{

using namespace orproblems::capacitatedvehiclerouting;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(
            const Instance& instance,
            double dummy_column_objective_coefficient):
        instance_(instance),
        visited_customers_(instance.number_of_locations(), 0),
        dummy_column_objective_coefficient_(dummy_column_objective_coefficient)
    { }

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<Demand> visited_customers_;

    std::vector<LocationId> espp2vrp_;

    treesearchsolver::NodeId bs_size_of_the_queue_ = 128;

    double dummy_column_objective_coefficient_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    columngenerationsolver::Parameters p(instance.number_of_locations() - 1);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Min;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row bounds.
    for (LocationId location_id = 0;
            location_id < instance.number_of_locations() - 1;
            ++location_id) {
        p.row_lower_bounds[location_id] = 1;
        p.row_upper_bounds[location_id] = 1;
        p.row_coefficient_lower_bounds[location_id] = 0;
        p.row_coefficient_upper_bounds[location_id] = 1;
    }
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = 3 * instance.maximum_distance();
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance, p.dummy_column_objective_coefficient));
    return p;
}

std::vector<ColIdx> PricingSolver::initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns)
{
    std::fill(visited_customers_.begin(), visited_customers_.end(), 0);
    for (auto p: fixed_columns) {
        const Column& column = columns[p.first];
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
            RowIdx row_index = column.row_indices[row_pos];
            Value row_coefficient = column.row_coefficients[row_pos];
            if (row_coefficient < 0.5)
                continue;
            // row_index + 1 since there is not constraint for location 0 which
            // is the depot.
            visited_customers_[row_index + 1] = 1;
        }
    }
    return {};
}

struct ColumnExtra
{
    std::vector<LocationId> route;
};

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
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

    LocationId espp_n = espp2vrp_.size();
    espprc::Instance instance_espp(espp_n);
    for (LocationId espp_location_id = 0;
            espp_location_id < espp_n;
            ++espp_location_id) {
        LocationId location_id = espp2vrp_[espp_location_id];
        instance_espp.set_location(
                espp_location_id,
                instance_.demand(location_id),
                ((location_id != 0)? duals[location_id - 1]: 0));
        for (LocationId espp_location_id_2 = 0;
                espp_location_id_2 < espp_n;
                ++espp_location_id_2) {
            if (espp_location_id_2 == espp_location_id)
                continue;
            LocationId location_id_2 = espp2vrp_[espp_location_id_2];
            instance_espp.set_distance(
                    espp_location_id,
                    espp_location_id_2,
                    instance_.distance(location_id, location_id_2));
        }
    }

    std::vector<Column> columns;

    // Solve subproblem instance.
    espprc::BranchingScheme branching_scheme(instance_espp);
    for (;;) {
        bool ok = false;

        treesearchsolver::IterativeBeamSearchOptionalParameters<espprc::BranchingScheme> espp_parameters;
        espp_parameters.maximum_size_of_the_solution_pool = 100;
        espp_parameters.minimum_size_of_the_queue = bs_size_of_the_queue_;
        espp_parameters.maximum_size_of_the_queue = bs_size_of_the_queue_;
        //espp_parameters.info.set_verbose(true);
        auto espp_output = treesearchsolver::iterative_beam_search(
                branching_scheme, espp_parameters);

        // Retrieve column.
        LocationId i = 0;
        for (const std::shared_ptr<espprc::BranchingScheme::Node>& node:
                espp_output.solution_pool.solutions()) {
            if (i > 2 * espp_n)
                break;
            if (node->j == 0)
                continue;
            std::vector<LocationId> solution; // Without the depot.
            for (auto node_tmp = node;
                    node_tmp->father != nullptr;
                    node_tmp = node_tmp->father)
                solution.push_back(espp2vrp_[node_tmp->j]);
            std::reverse(solution.begin(), solution.end());
            i += solution.size();

            Column column;
            LocationId location_id_prev = 0;
            for (LocationId location_id: solution) {
                column.row_indices.push_back(location_id - 1);
                column.row_coefficients.push_back(1);
                column.objective_coefficient += instance_.distance(location_id_prev, location_id);
                location_id_prev = location_id;
            }
            column.objective_coefficient += instance_.distance(location_id_prev, 0);
            ColumnExtra extra {solution};
            column.extra = std::shared_ptr<void>(new ColumnExtra(extra));
            columns.push_back(column);

            if (columngenerationsolver::compute_reduced_cost(column, duals)
                    <= -dummy_column_objective_coefficient_ * 10e-9) {
                ok = true;
            }
        }
        if (ok) {
            break;
        } else {
            if (bs_size_of_the_queue_ < 1024) {
                bs_size_of_the_queue_ *= 2;
            } else {
                break;
            }
        }
    }
    return columns;
}

}

}

