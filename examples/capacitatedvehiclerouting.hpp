#pragma once

#include "columngenerationsolver/columngenerationsolver.hpp"

#include "optimizationtools/utils.hpp"
#include "knapsacksolver/algorithms/minknap.hpp"

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
    Distance x;
    Distance y;
    Demand demand;
};

class Instance
{

public:

    Instance(LocationId n):
        locations_(n),
        distances_(n, std::vector<Distance>(n, -1)) { }
    void set_demand(LocationId j, Demand q) { locations_[j].demand = q; }
    void set_xy(LocationId j, Distance x, Distance y)
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
    Demand maximum_distance() const { return distance_max_; }

private:

    void read_cvrplib(std::ifstream& file)
    {
        std::string tmp;
        std::vector<std::string> line;
        LocationId n = -1;
        std::string edge_weight_type;
        for (;;) {
            getline(file, tmp);
            line = optimizationtools::split(tmp, ' ');
            if (line[0] == "DIMENSION") {
                n = std::stol(line[2]);
                locations_ = std::vector<Location>(n);
                distances_ = std::vector<std::vector<Distance>>(n, std::vector<Distance>(n, -1));
            } else if (line[0] == "EDGE_WEIGHT_TYPE") {
                edge_weight_type = line[2];
            } else if (line[0] == "CAPACITY") {
                Demand c = std::stol(line[2]);
                set_demand(0, c);
            } else if (line[0] == "NODE_COORD_SECTION") {
                for (LocationId j = 0; j < n; ++j) {
                    getline(file, tmp);
                    line = optimizationtools::split(tmp, ' ');
                    Distance x = std::stol(line[1]);
                    Distance y = std::stol(line[2]);
                    set_xy(j, x, y);
                }
            } else if (line[0] == "EOF") {
                break;
            }
        }

        // Compute distances.
        if (edge_weight_type == "EUC_2D") {
            for (LocationId j1 = 0; j1 < n; ++j1) {
                for (LocationId j2 = j1 + 1; j2 < n; ++j2) {
                    Distance xd = x(j2) - x(j1);
                    Distance yd = y(j2) - y(j1);
                    Distance d = (Distance)(std::sqrt(xd * xd + yd * yd));
                    set_distance(j1, j2, d);
                }
            }
        } else {
            std::cerr << "\033[31m" << "ERROR, EDGE_WEIGHT_TYPE \"" << edge_weight_type << "\" not implemented." << "\033[0m" << std::endl;
        }
    }

    std::vector<Location> locations_;
    std::vector<std::vector<Distance>> distances_;
    Demand distance_max_ = 0;

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

columngenerationsolver::Parameters get_parameters(
        const Instance& instance,
        columngenerationsolver::LinearProgrammingSolver linear_programming_solver)
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
    p.linear_programming_solver = linear_programming_solver;
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
            visited_customers_[row_index + 1] = 1;
        }
    }
    return {};
}

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    LocationId n = instance_.location_number();
    knapsacksolver::Profit mult = 10000;

    // Build knapsack instance.
    espprc2cvrp_.clear();
    for (LocationId j = 1; j < n; ++j) {
        if (visited_customers_[j] == 1)
            continue;
        knapsacksolver::Profit profit = std::floor(mult * duals[j]);
        if (profit <= 0)
            continue;
        espprc2cvrp_.push_back(j);
    }
    LocationId n_espprc = espprc2cvrp_.size();
    // TODO Create RCSPP instance of size n_espprc.
    for (LocationId j_espprc = 0; j_espprc < n_espprc; ++j_espprc) {
        LocationId j = espprc2cvrp_[j_espprc];
        // TODO Set x, y and demand of j_espprc to instance_.x(j),
        // instance_.y(j) and instance.demand(j).
        for (LocationId j2_espprc = 0; j2_espprc < n_espprc; ++j2_espprc) {
            LocationId j2 = espprc2cvrp_[j2_espprc];
            Distance d = mult * instance_.distance(j, j2)
                - std::floor(mult * duals[j]);
            (void)d;
            // TODO Set distance d between j_espprc and j2_espprc.
        }
    }

    // TODO Solve RCSPP instance.
    //auto output_kp = knapsacksolver::minknap(instance_kp);
    std::vector<LocationId> solution = {}; // Without the depot.

    // Retrieve column.
    Column column;
    column.objective_coefficient = 1;
    std::vector<Demand> demands(instance_.location_number(), 0);
    for (LocationId j_espprc: solution) {
        column.row_indices.push_back(espprc2cvrp_[j_espprc]);
        column.row_coefficients.push_back(1);
    }
    return {column};
}

}

}
