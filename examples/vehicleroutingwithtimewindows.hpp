#pragma once

#include "columngenerationsolver/commons.hpp"

#include "examples/pricingsolver/espprctw.hpp"
#include "treesearchsolver/algorithms/iterative_beam_search.hpp"
#include "treesearchsolver/algorithms/a_star.hpp"
#include "treesearchsolver/algorithms/iterative_memory_bounded_a_star.hpp"

#include "optimizationtools/utils.hpp"

#include "pugixml.hpp"

/**
 * Vehicle Routing Problem with Time Windows.
 *
 * Input:
 * - m vehicles of capacity Q
 * - 1 depot
 * - n - 1 customer with a demand qⱼ, a service time sⱼ and a time window [rⱼ,
 *   dⱼ] (j = 2..n)
 * - A n×n symmetric matrix d specifying the times to travel between each pair
 *   of locations
 * Problem:
 * - find a set of routes that begin and end at the depot, such that
 *   - each customer is visited on exactly one route
 *   - each customer is visited during its time window
 *   - the total demand by the customers assigned to a route does not exceed
 *     the vehicle capacity Q.
 * Objective:
 * - minimize the total combined cost of the routes. The cost of a route is the
 *   sum of travel times between each pair of consecutive location of the
 *   route, including the departure and the arrival to the depot, excluding the
 *   waiting times.
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
 * Therefore, finding a variable of minium reduced cost reduces to solving
 * an Elementary Shortest Path Problems with Resource Constraints and Time
 * Windows.
 *
 */


namespace columngenerationsolver
{

namespace vehicleroutingwithtimewindowssolver
{

typedef int64_t LocationId;
typedef int64_t VehicleId;
typedef int64_t RouteId;
typedef double Demand;
typedef double Time;

struct Location
{
    double x;
    double y;
    Demand demand;
    Time release_date = 0;
    Time deadline = 0;
    Time service_time = 0;
};

class Instance
{

public:

    Instance(LocationId n):
        locations_(n),
        times_(n, std::vector<Time>(n, -1)) { }
    void set_vehicle_number(VehicleId m) { vehicle_number_ = m; }
    void set_capacity(Demand capacity) { locations_[0].demand = capacity; }
    void set_location(
            LocationId j,
            Demand demand,
            Time release_date,
            Time deadline,
            Time service_time)
    {
        locations_[j].demand = demand;
        locations_[j].release_date = release_date;
        locations_[j].deadline = deadline;
        locations_[j].service_time = service_time;
        service_time_max_ = std::max(service_time_max_, service_time);
    }
    void set_xy(LocationId j, double x, double y)
    {
        locations_[j].x = x;
        locations_[j].y = y;
    }
    void set_time(LocationId j1, LocationId j2, Time d)
    {
        times_[j1][j2] = d;
        times_[j2][j1] = d;
        time_max_ = std::max(time_max_, d);
    }

    Instance(std::string instance_path, std::string format = "")
    {
        std::ifstream file(instance_path);
        if (!file.good()) {
            std::cerr << "\033[31m" << "ERROR, unable to open file \"" << instance_path << "\"" << "\033[0m" << std::endl;
            assert(false);
            return;
        }
        if (format == "" || format == "dimacs2021") {
            read_dimacs2021(file);
        } else if (format == "vrprep") {
            read_vrprep(file);
        } else {
            std::cerr << "\033[31m" << "ERROR, unknown instance format \"" << format << "\"" << "\033[0m" << std::endl;
        }
        file.close();
    }

    virtual ~Instance() { }

    VehicleId vehicle_number() const { return vehicle_number_; }
    LocationId location_number() const { return locations_.size(); }
    Demand capacity() const { return locations_[0].demand; }
    inline const Location& location(LocationId j) const { return locations_[j]; }
    Time time(LocationId j1, LocationId j2) const { return times_[j1][j2]; }
    Time maximum_time() const { return time_max_; }
    Time maximum_service_time() const { return service_time_max_; }

private:

    void read_dimacs2021(std::ifstream& file)
    {
        std::string tmp;
        file >> tmp >> tmp >> tmp >> tmp;

        // Read location number.
        VehicleId m = -1;
        file >> m;
        set_vehicle_number(m);

        // Read capacity.
        Demand capacity = -1;
        file >> capacity;

        file
            >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp
            >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp
            ;

        // Read locations.
        LocationId j;
        double x = -1;
        double y = -1;
        Demand demand = -1;
        Time release_date = -1;
        Time deadline = -1;
        Time service_time = -1;
        while (file >> j >> x >> y >> demand >> release_date >> deadline >> service_time) {
            locations_.push_back({});
            set_xy(j, x, y);
            set_location(j, demand, release_date, deadline, service_time);
        }
        set_capacity(capacity);
        LocationId n = locations_.size();

        // Compute times.
        times_.resize(n, std::vector<Time>(n, -1));
        for (LocationId j1 = 0; j1 < n; ++j1) {
            for (LocationId j2 = j1 + 1; j2 < n; ++j2) {
                double xd = location(j2).x - location(j1).x;
                double yd = location(j2).y - location(j1).y;
                double d = std::sqrt(xd * xd + yd * yd);
                d = std::round(d * 10) / 10;
                set_time(j1, j2, d);
            }
        }
    }

    void read_vrprep(std::ifstream& file)
    {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load(file);
        if (!result) {
            std::cerr << "\033[31m" << "ERROR, XML parsed with errors \"" << result.description() << "\"" << "\033[0m" << std::endl;
            return;
        }

        // Number of locations.
        LocationId n = 0;
        for (pugi::xml_node child = doc.child("instance").child("network").child("nodes").first_child();
                child; child = child.next_sibling())
            n++;
        locations_.resize(n),
        times_.resize(n, std::vector<Time>(n, -1));

        // Coordinates.
        for (pugi::xml_node child = doc.child("instance").child("network").child("nodes").first_child();
                child; child = child.next_sibling()) {
            LocationId j = child.attribute("id").as_int();
            double x = std::stod(child.child("cx").child_value());
            double y = std::stod(child.child("cy").child_value());
            set_xy(j, x, y);
        }

        // Location attributes.
        for (pugi::xml_node child = doc.child("instance").child("requests").first_child();
                child; child = child.next_sibling()) {
            LocationId j = child.attribute("node").as_int();
            Demand demand = std::stod(child.child("quantity").child_value());
            Time release_date = std::stod(child.child("tw").child("start").child_value());
            Time deadline = std::stod(child.child("tw").child("end").child_value());
            Time service_time = std::stod(child.child("service_time").child_value());
            set_location(j, demand, release_date, deadline, service_time);
        }

        // Vehicle number.
        VehicleId m = doc.child("instance").child("fleet").child("vehicle_profile").attribute("number").as_int();
        set_vehicle_number(m);
        // Vehicle capacity.
        Demand c = std::stod(doc.child("instance").child("fleet").child("vehicle_profile").child("capacity").child_value());
        set_capacity(c);

        // Compute times.
        if (doc.child("instance").child("network").child("euclidean")) {
            double dec = std::stod(doc.child("instance").child("network").child("decimals").child_value());
            for (LocationId j1 = 0; j1 < n; ++j1) {
                for (LocationId j2 = j1 + 1; j2 < n; ++j2) {
                    double xd = location(j2).x - location(j1).x;
                    double yd = location(j2).y - location(j1).y;
                    double d = std::sqrt(xd * xd + yd * yd);
                    d = std::round(d * std::pow(10, dec)) / std::pow(10, dec);
                    set_time(j1, j2, d);
                }
            }
        } else {
            std::cerr << "\033[31m" << "ERROR, computing times." << "\033[0m" << std::endl;
        }
    }

    std::vector<Location> locations_;
    std::vector<std::vector<Time>> times_;
    VehicleId vehicle_number_ = 0;
    Time time_max_ = 0;
    Time service_time_max_ = 0;

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

    std::vector<LocationId> espprctw2cvrp_;

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
    espprctw2cvrp_.clear();
    espprctw2cvrp_.push_back(0);
    for (LocationId j = 1; j < n; ++j) {
        if (visited_customers_[j] == 1)
            continue;
        espprctw2cvrp_.push_back(j);
    }
    LocationId n_espprctw = espprctw2cvrp_.size();
    if (n_espprctw == 1)
        return {};
    espprctw::Instance instance_espprctw(n_espprctw);
    for (LocationId j_espprctw = 0; j_espprctw < n_espprctw; ++j_espprctw) {
        LocationId j = espprctw2cvrp_[j_espprctw];
        instance_espprctw.set_location(
                j_espprctw,
                instance_.location(j).demand,
                ((j != 0)? duals[j]: 0),
                instance_.location(j).release_date,
                instance_.location(j).deadline,
                instance_.location(j).service_time);
        for (LocationId j2_espprctw = 0; j2_espprctw < n_espprctw; ++j2_espprctw) {
            if (j2_espprctw == j_espprctw)
                continue;
            LocationId j2 = espprctw2cvrp_[j2_espprctw];
            instance_espprctw.set_time(j_espprctw, j2_espprctw, instance_.time(j, j2));
        }
    }

    // Solve subproblem instance.
    espprctw::BranchingScheme branching_scheme(instance_espprctw);
    treesearchsolver::IterativeBeamSearchOptionalParameters parameters_espprctw;
    parameters_espprctw.solution_pool_size_max = 100;
    parameters_espprctw.queue_size_min = 512;
    parameters_espprctw.queue_size_max = 512;
    auto output_espprctw = treesearchsolver::iterativebeamsearch(
            branching_scheme, parameters_espprctw);

    // Retrieve column.
    std::vector<Column> columns;
    LocationId i = 0;
    for (const std::shared_ptr<espprctw::BranchingScheme::Node>& node:
            output_espprctw.solution_pool.solutions()) {
        if (i > 2 * n_espprctw)
            break;
        std::vector<LocationId> solution; // Without the depot.
        if (node->j != 0) {
            for (auto node_tmp = node; node_tmp->father != nullptr; node_tmp = node_tmp->father)
                solution.push_back(espprctw2cvrp_[node_tmp->j]);
            std::reverse(solution.begin(), solution.end());
        }
        i += solution.size();

        Column column;
        column.objective_coefficient = node->cost + instance_espprctw.time(node->j, 0);
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
