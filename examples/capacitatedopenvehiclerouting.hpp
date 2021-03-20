#pragma once

#include "columngenerationsolver/commons.hpp"

#include "examples/pricingsolver/eospprc.hpp"
#include "treesearchsolver/algorithms/iterative_beam_search.hpp"
#include "treesearchsolver/algorithms/a_star.hpp"
#include "treesearchsolver/algorithms/iterative_memory_bounded_a_star.hpp"
#include "optimizationtools/utils.hpp"

/**
 * Capacitated Open Vehicle Routing Problem.
 *
 * Input:
 * - m vehicles of capacity Q
 * - A maximum route length L
 * - 1 depot
 * - n - 1 customer with demand qⱼ (j = 2..n)
 * - A n×n symmetric matrix d specifying the distances to travel between each
 *   pair of locations
 * Problem:
 * - find a set of at most m paths that begin (but does not end) at the depot,
 *   such that
 *   - each customer is visited on exactly one path
 *   - the total demand by the customers assigned to a path does not exceed
 *     the vehicle capacity Q
 *   - the total length of each route does not exceed the maximum route length
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
 * Therefore, finding a variable of minium reduced cost reduces to solving
 * an Elementary Open Shortest Path Problems with Resource Constraints.
 *
 */


namespace columngenerationsolver
{

namespace capacitatedopenvehicleroutingsolver
{

typedef int64_t LocationId;
typedef int64_t VehicleId;
typedef int64_t RouteId;
typedef int64_t Demand;
typedef double Distance;

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
        distances_(n, std::vector<Distance>(n, -1)),
        vehicle_number_(n) { }
    void set_vehicle_number(VehicleId m) { vehicle_number_ = m; }
    void set_maximum_route_length(Distance maximum_route_length) { maximum_route_length_ = maximum_route_length; }
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
        if (format == "" || format == "vrplib") {
            read_vrplib(file);
        } else {
            std::cerr << "\033[31m" << "ERROR, unknown instance format \"" << format << "\"" << "\033[0m" << std::endl;
        }
        file.close();
    }

    virtual ~Instance() { }

    VehicleId vehicle_number() const { return vehicle_number_; }
    LocationId location_number() const { return locations_.size(); }
    Distance maximum_route_length() const { return maximum_route_length_; }
    Demand capacity() const { return locations_[0].demand; }
    Demand demand(LocationId j) const { return locations_[j].demand; }
    Distance x(LocationId j) const { return locations_[j].x; }
    Distance y(LocationId j) const { return locations_[j].y; }
    Distance distance(LocationId j1, LocationId j2) const { return distances_[j1][j2]; }
    Distance maximum_distance() const { return distance_max_; }
    Distance bound() const { return powf(10.0f, ceil(log10f(location_number() * maximum_distance()))); }

private:

    void read_vrplib(std::ifstream& file)
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
                vehicle_number_ = n;
            } else if (tmp.rfind("EDGE_WEIGHT_TYPE", 0) == 0) {
                edge_weight_type = line.back();
            } else if (tmp.rfind("DISTANCE", 0) == 0) {
                Distance l = std::stol(line.back());
                set_maximum_route_length(l * 0.9);
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
                    double xd = x(j2) - x(j1);
                    double yd = y(j2) - y(j1);
                    //Distance d = std::round(std::sqrt(xd * xd + yd * yd));
                    Distance d = std::sqrt(xd * xd + yd * yd);
                    set_distance(j1, j2, d);
                }
            }
        } else {
            std::cerr << "\033[31m" << "ERROR, EDGE_WEIGHT_TYPE \"" << edge_weight_type << "\" not implemented." << "\033[0m" << std::endl;
        }
    }

    std::vector<Location> locations_;
    std::vector<std::vector<Distance>> distances_;
    VehicleId vehicle_number_ = 0;
    Distance maximum_route_length_ = std::numeric_limits<Distance>::infinity();;
    Distance distance_max_ = 0;

};

static std::ostream& operator<<(std::ostream &os, const Instance& instance)
{
    os << "location number " << instance.location_number() << std::endl;
    os << "vehicle number " << instance.vehicle_number() << std::endl;
    os << "capacity " << instance.capacity() << std::endl;
    os << "maximum route length " << instance.maximum_route_length() << std::endl;
    os << "bound " << instance.bound() << std::endl;
    for (LocationId j = 0; j < instance.location_number(); ++j)
        os << "location " << j
            << " d " << instance.demand(j)
            << std::endl;
    return os;
}

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

    std::vector<LocationId> espp2vrp_;

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
    LocationId n = instance_.location_number();

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
    parameters_espp.solution_pool_size_max = 100;
    parameters_espp.queue_size_min = 512;
    parameters_espp.queue_size_max = 512;
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
