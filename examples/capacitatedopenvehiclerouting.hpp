#pragma once

/**
 * Capacitated Open Vehicle Routing Problem.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/capacitatedopenvehiclerouting.hpp
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
 * 0 <= ∑ₖ yᵏ <= m
 *                                                 (not more then m vehicles)
 *                                                           Dual variable: u
 * 1 <= ∑ₖ xⱼᵏ yᵏ <= 1     for all customers j
 *                                    (each customer is visited exactly once)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of negative reduced cost.
 * The reduced cost of a variable yᵏ is given by:
 * rc(yᵏ) = dᵏ - ∑ⱼ xⱼᵏ vⱼ
 *
 * Therefore, finding a variable of minimum reduced cost reduces to solving
 * an Elementary Open Shortest Path Problems with Resource Constraints.
 *
 */

#include "columngenerationsolver/commons.hpp"

#include "examples/pricingsolver/eospprc.hpp"

#include "orproblems/capacitatedopenvehiclerouting.hpp"

#include "treesearchsolver/algorithms/iterative_beam_search.hpp"
#include "treesearchsolver/algorithms/a_star.hpp"
#include "treesearchsolver/algorithms/iterative_memory_bounded_a_star.hpp"

#include "optimizationtools/utils.hpp"

namespace columngenerationsolver
{

namespace capacitatedopenvehiclerouting
{

using namespace orproblems::capacitatedopenvehiclerouting;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        visited_customers_(instance.number_of_locations(), 0)
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

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    LocationId n = instance.number_of_locations();
    columngenerationsolver::Parameters p(n);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Min;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row bounds.
    p.row_lower_bounds[0] = 0;
    p.row_upper_bounds[0] = instance.number_of_vehicles();
    p.row_coefficient_lower_bounds[0] = 1;
    p.row_coefficient_upper_bounds[0] = 1;
    for (LocationId j = 1; j < n; ++j) {
        p.row_lower_bounds[j] = 1;
        p.row_upper_bounds[j] = 1;
        p.row_coefficient_lower_bounds[j] = 0;
        p.row_coefficient_upper_bounds[j] = 1;
    }
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = 3 * instance.maximum_distance();
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));
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
            if (row_index == 0)
                continue;
            if (row_coefficient < 0.5)
                continue;
            visited_customers_[row_index] = 1;
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
    LocationId n = instance_.number_of_locations();

    // Build subproblem instance.
    espp2vrp_.clear();
    espp2vrp_.push_back(0);
    for (LocationId j = 1; j < n; ++j) {
        if (visited_customers_[j] == 1)
            continue;
        espp2vrp_.push_back(j);
    }
    LocationId n_espp = espp2vrp_.size();
    if (n_espp == 1)
        return {};
    eospprc::Instance instance_espp(n_espp);
    instance_espp.set_maximum_route_length(instance_.maximum_route_length());
    for (LocationId j_espp = 0; j_espp < n_espp; ++j_espp) {
        LocationId j = espp2vrp_[j_espp];
        instance_espp.set_location(
                j_espp,
                instance_.demand(j),
                ((j != 0)? duals[j]: 0));
        for (LocationId j2_espp = 0; j2_espp < n_espp; ++j2_espp) {
            if (j2_espp == j_espp)
                continue;
            LocationId j2 = espp2vrp_[j2_espp];
            instance_espp.set_distance(j_espp, j2_espp, instance_.distance(j, j2));
        }
    }

    // Solve subproblem instance.
    eospprc::BranchingScheme branching_scheme(instance_espp);
    treesearchsolver::IterativeBeamSearchOptionalParameters parameters_espp;
    parameters_espp.maximum_size_of_the_solution_pool = 100;
    parameters_espp.minimum_size_of_the_queue = 512;
    parameters_espp.maximum_size_of_the_queue = 512;
    //parameters_espp.info.set_verbose(true);
    auto output_espp = treesearchsolver::iterativebeamsearch(
            branching_scheme, parameters_espp);

    // Retrieve column.
    std::vector<Column> columns;
    LocationId i = 0;
    for (const std::shared_ptr<eospprc::BranchingScheme::Node>& node:
            output_espp.solution_pool.solutions()) {
        if (i > 2 * n_espp)
            break;
        std::vector<LocationId> solution; // Without the depot.
        if (node->j != 0) {
            for (auto node_tmp = node; node_tmp->father != nullptr; node_tmp = node_tmp->father)
                solution.push_back(espp2vrp_[node_tmp->j]);
            std::reverse(solution.begin(), solution.end());
        }
        i += solution.size();

        Column column;
        column.objective_coefficient = node->length;
        column.row_indices.push_back(0);
        column.row_coefficients.push_back(1);
        for (LocationId j: solution) {
            column.row_indices.push_back(j);
            column.row_coefficients.push_back(1);
        }
        ColumnExtra extra {solution};
        column.extra = std::shared_ptr<void>(new ColumnExtra(extra));
        columns.push_back(column);
    }
    return columns;
}

}

}
