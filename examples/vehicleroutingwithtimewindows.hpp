#pragma once

/**
 * Vehicle Routing Problem with Time Windows.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/vehicleroutingwithtimewindows.hpp
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
 * rc(yᵏ) = dᵏ - u - ∑ⱼ xⱼᵏ vⱼ
 *
 * Therefore, finding a variable of minimum reduced cost reduces to solving
 * an Elementary Shortest Path Problems with Resource Constraints and Time
 * Windows.
 *
 */

#include "columngenerationsolver/commons.hpp"

#include "examples/pricingsolver/espprctw.hpp"

#include "orproblems/vehicleroutingwithtimewindows.hpp"

#include "treesearchsolver/algorithms/iterative_beam_search.hpp"
#include "treesearchsolver/algorithms/a_star.hpp"
#include "treesearchsolver/algorithms/iterative_memory_bounded_a_star.hpp"

#include "optimizationtools/utils.hpp"

#include "pugixml.hpp"

namespace columngenerationsolver
{

namespace vehicleroutingwithtimewindows
{

using namespace orproblems::vehicleroutingwithtimewindows;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        visited_customers_(instance.location_number(), 0)
    { }

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<Demand> visited_customers_;

    std::vector<LocationId> espp2cvrp_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    LocationId n = instance.location_number();
    columngenerationsolver::Parameters p(n);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Min;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row bounds.
    p.row_lower_bounds[0] = 0;
    p.row_upper_bounds[0] = instance.vehicle_number();
    p.row_coefficient_lower_bounds[0] = 1;
    p.row_coefficient_upper_bounds[0] = 1;
    for (LocationId j = 1; j < n; ++j) {
        p.row_lower_bounds[j] = 1;
        p.row_upper_bounds[j] = 1;
        p.row_coefficient_lower_bounds[j] = 0;
        p.row_coefficient_upper_bounds[j] = 1;
    }
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = 3 * instance.maximum_time() + instance.maximum_service_time();
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
    LocationId n = instance_.location_number();

    // Build subproblem instance.
    espp2cvrp_.clear();
    espp2cvrp_.push_back(0);
    for (LocationId j = 1; j < n; ++j) {
        if (visited_customers_[j] == 1)
            continue;
        espp2cvrp_.push_back(j);
    }
    LocationId n_espp = espp2cvrp_.size();
    if (n_espp == 1)
        return {};
    espprctw::Instance instance_espp(n_espp);
    for (LocationId j_espp = 0; j_espp < n_espp; ++j_espp) {
        LocationId j = espp2cvrp_[j_espp];
        instance_espp.set_location(
                j_espp,
                instance_.location(j).demand,
                ((j != 0)? duals[j]: 0),
                instance_.location(j).release_date,
                instance_.location(j).deadline,
                instance_.location(j).service_time);
        for (LocationId j2_espp = 0; j2_espp < n_espp; ++j2_espp) {
            if (j2_espp == j_espp)
                continue;
            LocationId j2 = espp2cvrp_[j2_espp];
            instance_espp.set_time(j_espp, j2_espp, instance_.time(j, j2));
        }
    }

    // Solve subproblem instance.
    espprctw::BranchingScheme branching_scheme(instance_espp);
    treesearchsolver::IterativeBeamSearchOptionalParameters parameters_espp;
    parameters_espp.solution_pool_size_max = 100;
    parameters_espp.queue_size_min = 512;
    parameters_espp.queue_size_max = 512;
    //parameters_espp.info.set_verbose(true);
    auto output_espp = treesearchsolver::iterativebeamsearch(
            branching_scheme, parameters_espp);

    // Retrieve column.
    std::vector<Column> columns;
    LocationId i = 0;
    for (const std::shared_ptr<espprctw::BranchingScheme::Node>& node:
            output_espp.solution_pool.solutions()) {
        if (i > 2 * n_espp)
            break;
        std::vector<LocationId> solution; // Without the depot.
        if (node->j != 0) {
            for (auto node_tmp = node; node_tmp->father != nullptr; node_tmp = node_tmp->father)
                solution.push_back(espp2cvrp_[node_tmp->j]);
            std::reverse(solution.begin(), solution.end());
        }
        i += solution.size();

        Column column;
        column.objective_coefficient = node->cost + instance_espp.time(node->j, 0);
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

