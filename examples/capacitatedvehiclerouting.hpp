#pragma once

#include "columngenerationsolver/commons.hpp"

#include "examples/pricingsolver/espprc.hpp"
#include "treesearchsolver/algorithms/iterative_beam_search.hpp"
#include "treesearchsolver/algorithms/a_star.hpp"
#include "treesearchsolver/algorithms/iterative_memory_bounded_a_star.hpp"
#include "optimizationtools/utils.hpp"

/**
 * Capacitated Vehicle Routing Problem.
 *
 * Input:
 * - vehicles of capacity Q
 * - 1 depot
 * - n - 1 customer with demand qⱼ (j = 2..n)
 * - A n×n symmetric matrix d specifying the distances to travel between each
 *   pair of locations
 * Problem:
 * - find a set of routes that begin and end at the depot, such that each
 *   customer is visited on exactly one route and the total demand by the
 *   customers assigned to a route does not exceed the vehicle capacity Q.
 * Objective:
 * - minimize the total combined distance of the routes.
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
 * Therefore, finding a variable of minium reduced cost reduces to solving
 * an Elementary Shortest Path Problems with Resource Constraints.
 *
 */


namespace columngenerationsolver
{

namespace capacitatedvehicleroutingsolver
{

typedef int64_t LocationId;
typedef int64_t RouteId;
typedef int64_t Demand;
typedef int64_t Distance;

struct Location
{
    double x;
    double y;
    Demand demand;
};

class Instance
{

public:

    Instance(LocationId n):
        locations_(n),
        distances_(n, std::vector<Distance>(n, -1)) { }
    void set_demand(LocationId j, Demand q) { locations_[j].demand = q; }
    void set_xy(LocationId j, double x, double y)
    {
        locations_[j].x = x;
        locations_[j].y = y;
    }
    void set_distance(LocationId j1, LocationId j2, Distance d)
    {
        distances_[j1][j2] = d;
        distances_[j2][j1] = d;
        distance_max_ = std::max(distance_max_, d);
    }

    Instance(std::string instance_path, std::string format = "")
    {
        std::ifstream file(instance_path);
        if (!file.good()) {
            std::cerr << "\033[31m" << "ERROR, unable to open file \"" << instance_path << "\"" << "\033[0m" << std::endl;
            assert(false);
            return;
        }
        if (format == "" || format == "cvrplib") {
            read_cvrplib(file);
        } else {
            std::cerr << "\033[31m" << "ERROR, unknown instance format \"" << format << "\"" << "\033[0m" << std::endl;
        }
        file.close();
    }

    virtual ~Instance() { }

    LocationId location_number() const { return locations_.size(); }
    Demand capacity() const { return locations_[0].demand; }
    Demand demand(LocationId j) const { return locations_[j].demand; }
    Distance x(LocationId j) const { return locations_[j].x; }
    Distance y(LocationId j) const { return locations_[j].y; }
    Distance distance(LocationId j1, LocationId j2) const { return distances_[j1][j2]; }
    Distance maximum_distance() const { return distance_max_; }

private:

    void read_cvrplib(std::ifstream& file)
    {
        std::string tmp;
        std::vector<std::string> line;
        LocationId n = -1;
        std::string edge_weight_type;
        while (getline(file, tmp)) {
            replace(begin(tmp), end(tmp), '\t', ' ');
            line = optimizationtools::split(tmp, ' ');
            if (line.empty()) {
            } else if (tmp.rfind("NAME", 0) == 0) {
            } else if (tmp.rfind("COMMENT", 0) == 0) {
            } else if (tmp.rfind("TYPE", 0) == 0) {
            } else if (tmp.rfind("DEPOT_SECTION", 0) == 0) {
                LocationId j_tmp;
                file >> j_tmp >> j_tmp;
            } else if (tmp.rfind("DIMENSION", 0) == 0) {
                n = std::stol(line.back());
                locations_ = std::vector<Location>(n);
                distances_ = std::vector<std::vector<Distance>>(n, std::vector<Distance>(n, -1));
            } else if (tmp.rfind("EDGE_WEIGHT_TYPE", 0) == 0) {
                edge_weight_type = line.back();
            } else if (tmp.rfind("CAPACITY", 0) == 0) {
                Demand c = std::stol(line.back());
                set_demand(0, c);
            } else if (tmp.rfind("NODE_COORD_SECTION", 0) == 0) {
                LocationId j_tmp;
                double x = -1;
                double y = -1;
                for (LocationId j = 0; j < n; ++j) {
                    file >> j_tmp >> x >> y;
                    set_xy(j, x, y);
                }
            } else if (tmp.rfind("DEMAND_SECTION", 0) == 0) {
                LocationId j_tmp = -1;
                Demand demand = -1;
                for (LocationId j = 0; j < n; ++j) {
                    file >> j_tmp >> demand;
                    if (j != 0)
                        set_demand(j, demand);
                }
            } else if (line[0].rfind("EOF", 0) == 0) {
                break;
            } else {
                std::cerr << "\033[31m" << "ERROR, ENTRY \"" << line[0] << "\" not implemented." << "\033[0m" << std::endl;
            }
        }

        // Compute distances.
        if (edge_weight_type == "EUC_2D") {
            for (LocationId j1 = 0; j1 < n; ++j1) {
                for (LocationId j2 = j1 + 1; j2 < n; ++j2) {
                    Distance xd = x(j2) - x(j1);
                    Distance yd = y(j2) - y(j1);
                    Distance d = std::round(std::sqrt(xd * xd + yd * yd));
                    set_distance(j1, j2, d);
                }
            }
        } else {
            std::cerr << "\033[31m" << "ERROR, EDGE_WEIGHT_TYPE \"" << edge_weight_type << "\" not implemented." << "\033[0m" << std::endl;
        }
    }

    std::vector<Location> locations_;
    std::vector<std::vector<Distance>> distances_;
    Distance distance_max_ = 0;

};

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

    std::vector<LocationId> espprc2cvrp_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    LocationId n = instance.location_number();
    columngenerationsolver::Parameters p(n - 1);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Min;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row bounds.
    for (LocationId j = 0; j < n - 1; ++j) {
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
    LocationId n = instance_.location_number();

    // Build subproblem instance.
    espprc2cvrp_.clear();
    espprc2cvrp_.push_back(0);
    for (LocationId j = 1; j < n; ++j) {
        if (visited_customers_[j] == 1)
            continue;
        espprc2cvrp_.push_back(j);
    }
    LocationId n_espprc = espprc2cvrp_.size();
    espprc::Instance instance_espprc(n_espprc);
    instance_espprc.set_capacity(instance_.capacity());
    for (LocationId j_espprc = 0; j_espprc < n_espprc; ++j_espprc) {
        LocationId j = espprc2cvrp_[j_espprc];
        instance_espprc.set_demand(j_espprc, instance_.demand(j));
        for (LocationId j2_espprc = 0; j2_espprc < n_espprc; ++j2_espprc) {
            if (j2_espprc == j_espprc)
                continue;
            LocationId j2 = espprc2cvrp_[j2_espprc];
            espprc::Distance d = instance_.distance(j, j2);
            if (j2_espprc != 0)
                d -= duals[j2 - 1];
            instance_espprc.set_distance(j_espprc, j2_espprc, d);
            if (std::isnan(d))
                exit(1);
        }
    }

    // Solve subproblem instance.
    espprc::BranchingScheme branching_scheme(instance_espprc);
    treesearchsolver::IterativeBeamSearchOptionalParameters parameters_espprc;
    parameters_espprc.solution_pool_size_max = 100;
    //parameters_espprc.node_number_max = 1024 * instance_.location_number();
    parameters_espprc.queue_size_min = 128;
    parameters_espprc.queue_size_max = 128;
    //parameters_espprc.info.set_verbose(true);
    auto output_espprc = treesearchsolver::iterativebeamsearch(branching_scheme, parameters_espprc);

    // Retrieve column.
    std::vector<Column> columns;
    LocationId i = 0;
    for (const std::shared_ptr<espprc::BranchingScheme::Node>& node:
            output_espprc.solution_pool.solutions()) {
        if (i > 2 * n_espprc)
            break;
        std::vector<LocationId> solution; // Without the depot.
        Distance length = 0;
        if (node->j != 0) {
            auto node_tmp = node;
            length = instance_.distance(espprc2cvrp_[node_tmp->j], 0);
            while (node_tmp->j != 0) {
                LocationId j = espprc2cvrp_[node_tmp->j];
                LocationId j_prev = espprc2cvrp_[node_tmp->father->j];
                solution.push_back(j);
                length += instance_.distance(j_prev, j);
                node_tmp = node_tmp->father;
            }
            std::reverse(solution.begin(), solution.end());
        }
        i += solution.size();

        Column column;
        column.objective_coefficient = length;
        std::vector<Demand> demands(instance_.location_number(), 0);
        for (LocationId j: solution) {
            column.row_indices.push_back(j - 1);
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
