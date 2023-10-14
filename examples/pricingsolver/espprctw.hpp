/**
 * Elementary shortest path problem with resource constraint and time windows
 *
 * Input:
 * - n vertices with (j = 1..n):
 *   - a demand dⱼ
 *   - a profit pⱼ
 *   - a service time sⱼ
 *   - a time window [rⱼ, dⱼ]
 * - an n×n matrix containing the times between each pair of vertices (not
 *   necessarily symmetric, times may not be negative)
 * - a capacity c
 * Problem:
 * - find a tour from vertex 1 to vertex 1 such that:
 *   - each other vertex is visited at most once
 *   - each other vertex is visited during its time window
 *   - the total demand of the visited vertices does not exceed the capacity
 * Objective:
 * - minimize the total cost of the tour minus the profit of the visited
 *   vertices. The cost of a tour is the sum of travel times between each pair
 *   of consecutive vertices of the tour, including the departure and the
 *   arrival to the initial node, excluding the waiting times.
 *
 * Tree search:
 * - forward branching
 * - guide: cost + time to the closest next child - profit
 *
 */

#pragma once

#include "optimizationtools/utils/info.hpp"
#include "optimizationtools/utils/utils.hpp"
#include "optimizationtools/containers/sorted_on_demand_array.hpp"

#include "localsearchsolver/sequencing.hpp"

#include "boost/dynamic_bitset.hpp"

namespace columngenerationsolver
{

namespace espprctw
{

using LocationId = int64_t;
using LocationPos = int64_t;
using Demand = int64_t;
using Time = double;
using Profit = double;

struct Location
{
    Demand demand = 0;
    Time release_date = 0;
    Time deadline = 0;
    Time service_time = 0;
    Profit profit = 0;
};

class Instance
{

public:

    Instance(LocationId number_of_locations):
        locations_(number_of_locations),
        travel_times_(number_of_locations, std::vector<Time>(number_of_locations, -1))
    {
        for (LocationId location_id = 0;
                location_id < number_of_locations;
                ++location_id) {
            travel_times_[location_id][location_id] = std::numeric_limits<Time>::max();
        }
    }

    void set_capacity(Demand demand) { locations_[0].demand = demand; }

    void set_location(
            LocationId location_id,
            Demand demand,
            Profit profit,
            Time release_date,
            Time deadline,
            Time service_time)
    {
        locations_[location_id].demand = demand;
        locations_[location_id].profit = profit;
        locations_[location_id].release_date = release_date;
        locations_[location_id].deadline = deadline;
        locations_[location_id].service_time = service_time;
    }

    void set_travel_time(
            LocationId location_id_1,
            LocationId location_id_2,
            Time travel_time)
    {
        travel_times_[location_id_1][location_id_2] = travel_time;
    }

    inline LocationId number_of_vertices() const { return locations_.size(); }
    inline Time travel_time(LocationId j1, LocationId j2) const { return travel_times_[j1][j2]; }
    inline const Location& location(LocationId j) const { return locations_[j]; }
    inline Demand capacity() const { return locations_[0].demand; }

private:

    std::vector<Location> locations_;
    std::vector<std::vector<Time>> travel_times_;

};

class BranchingScheme
{

public:

    struct Node
    {
        std::shared_ptr<Node> father = nullptr;
        boost::dynamic_bitset<> available_vertices;
        LocationId j = 0;
        LocationId number_of_vertices = 1;
        Time cost = 0;
        Time time = 0;
        Profit profit = 0;
        Profit remaining_profit = 0;
        Demand demand = 0;
        Demand remaining_demand = 0;
        double guide = 0;
        LocationPos next_child_pos = 1;
    };

    BranchingScheme(const Instance& instance):
        instance_(instance) { }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingScheme::Node());
        r->available_vertices.resize(instance_.number_of_vertices(), true);
        r->available_vertices[0] = false;
        for (LocationId j = 0; j < instance_.number_of_vertices(); ++j) {
            r->remaining_demand += instance_.location(j).demand;
            r->remaining_profit += instance_.location(j).profit;
        }
        return r;
    }

    inline std::shared_ptr<Node> next_child(
            const std::shared_ptr<Node>& father) const
    {
        assert(!infertile(father));
        assert(!leaf(father));
        LocationId j_next = father->next_child_pos;
        const Location& location = instance_.location(j_next);
        // Update father
        father->next_child_pos++;
        if (!father->available_vertices[j_next])
            return nullptr;
        if (father->demand + location.demand > instance_.capacity())
            return nullptr;
        Time t = instance_.travel_time(father->j, j_next);
        Time s = std::max(father->time + t, location.release_date);
        if (s > location.deadline)
            return nullptr;

        // Compute new child.
        auto child = std::shared_ptr<Node>(new BranchingScheme::Node());
        child->father = father;
        child->available_vertices = father->available_vertices;
        child->available_vertices[j_next] = false;
        child->j = j_next;
        child->number_of_vertices = father->number_of_vertices + 1;
        child->demand = father->demand + location.demand;
        child->remaining_demand = father->remaining_demand - location.demand;
        child->time = s + location.service_time;
        child->cost = father->cost + t;
        child->profit = father->profit + location.profit;
        child->remaining_profit = father->remaining_profit - location.profit;
        for (LocationId j = 0; j < instance_.number_of_vertices(); ++j) {
            if (!child->available_vertices[j])
                continue;
            if (child->demand + instance_.location(j).demand > instance_.capacity()) {
                child->available_vertices[j] = false;
                child->remaining_demand -= instance_.location(j).demand;
                child->remaining_profit -= instance_.location(j).profit;
            } else if (child->time + instance_.travel_time(j_next, j) > instance_.location(j).deadline) {
                child->available_vertices[j] = false;
                child->remaining_demand -= instance_.location(j).demand;
                child->remaining_profit -= instance_.location(j).profit;
            }
        }
        // Guide.
        child->guide = child->cost - child->profit;
        return child;
    }

    inline bool infertile(
            const std::shared_ptr<Node>& node) const
    {
        assert(node != nullptr);
        return node->next_child_pos == instance_.number_of_vertices();
    }

    inline bool operator()(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        assert(!infertile(node_1));
        assert(!infertile(node_2));
        if (node_1->number_of_vertices != node_2->number_of_vertices)
            return node_1->number_of_vertices < node_2->number_of_vertices;
        if (node_1->guide != node_2->guide)
            return node_1->guide < node_2->guide;
        return node_1.get() < node_2.get();
    }

    inline bool leaf(
            const std::shared_ptr<Node>& node) const
    {
        return node->number_of_vertices == instance_.number_of_vertices();
    }

    bool bound(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_vertices == 1)
            return false;
        return node_1->cost + instance_.travel_time(node_1->j, 0) - node_1->profit - node_1->remaining_profit
            >= node_2->cost + instance_.travel_time(node_2->j, 0) - node_2->profit;
    }

    bool better(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        return node_1->cost + instance_.travel_time(node_1->j, 0) - node_1->profit
            < node_2->cost + instance_.travel_time(node_2->j, 0) - node_2->profit;
    }

    bool equals(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_vertices != node_2->number_of_vertices)
            return false;
        std::vector<bool> v(instance_.number_of_vertices(), false);
        for (auto node_tmp = node_1; node_tmp->father != nullptr; node_tmp = node_tmp->father)
            v[node_tmp->j] = true;
        for (auto node_tmp = node_1; node_tmp->father != nullptr; node_tmp = node_tmp->father)
            if (!v[node_tmp->j])
                return false;
        return true;
    }

    std::string display(const std::shared_ptr<Node>& node) const
    {
        if (node->j == 0)
            return "";
        std::stringstream ss;
        ss << node->cost + instance_.travel_time(node->j, 0) - node->profit
            << " (n" << node->number_of_vertices
            << " c" << node->cost + instance_.travel_time(node->j, 0)
            << " p" << node->profit
            << ")";
        return ss.str();
    }

    /**
     * Dominances.
     */

    inline bool comparable(
            const std::shared_ptr<Node>&) const
    {
        return true;
    }

    struct NodeHasher
    {
        std::hash<LocationId> hasher_1;
        std::hash<boost::dynamic_bitset<>> hasher_2;

        inline bool operator()(
                const std::shared_ptr<Node>& node_1,
                const std::shared_ptr<Node>& node_2) const
        {
            if (node_1->j != node_2->j)
                return false;
            return true;
        }

        inline std::size_t operator()(
                const std::shared_ptr<Node>& node) const
        {
            size_t hash = hasher_1(node->j);
            return hash;
        }
    };

    inline NodeHasher node_hasher() const { return NodeHasher(); }

    inline bool dominates(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->cost - node_1->profit <= node_2->cost - node_2->profit
                && node_1->time <= node_2->time
                && (node_1->demand <= node_2->demand
                    || node_1->demand + node_1->remaining_demand
                    <= instance_.capacity())
                && (node_1->available_vertices | node_2->available_vertices)
                == node_1->available_vertices)
            return true;
        return false;
    }

private:

    const Instance& instance_;

};

}

}

