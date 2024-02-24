/**
 * Elementary shortest path problem with resource constraint and time windows
 *
 * Input:
 * - n locations with (j = 1..n):
 *   - a demand dⱼ
 *   - a profit pⱼ
 *   - a service time sⱼ
 *   - a time window [rⱼ, dⱼ]
 * - an n×n matrix containing the times between each pair of locations (not
 *   necessarily symmetric, times may not be negative)
 * - a capacity c
 * Problem:
 * - find a tour from vertex 1 to vertex 1 such that:
 *   - each other vertex is visited at most once
 *   - each other vertex is visited during its time window
 *   - the total demand of the visited locations does not exceed the capacity
 * Objective:
 * - minimize the total cost of the tour minus the profit of the visited
 *   locations. The cost of a tour is the sum of travel times between each pair
 *   of consecutive locations of the tour, including the departure and the
 *   arrival to the initial node, excluding the waiting times.
 *
 * Tree search:
 * - forward branching
 * - guide: cost + time to the closest next child - profit
 *
 */

#pragma once

#include "treesearchsolver/common.hpp"

#include "boost/dynamic_bitset.hpp"

#include <cstdint>
#include <vector>
#include <limits>
#include <memory>
#include <cassert>
#include <iomanip>

namespace columngenerationsolver
{
namespace espprctw
{

using LocationId = int64_t;
using LocationPos = int64_t;
using Demand = int64_t;
using Time = double;
using Profit = double;

/**
 * Structure for a location.
 */
struct Location
{
    /** Demand. */
    Demand demand = 0;

    /** Release date. */
    Time release_date = 0;

    /** Deadline. */
    Time deadline = 0;

    /** Service time. */
    Time service_time = 0;

    /** Profit. */
    Profit profit = 0;
};

/**
 * Instance class for an 'espprctw' problem.
 */
class Instance
{

public:

    /** Get the number of locations. */
    inline LocationId number_of_locations() const { return locations_.size(); }

    /** Get the travel time between two locations. */
    inline Time travel_time(
            LocationId location_id_1,
            LocationId location_id_2) const
    {
        return travel_times_[location_id_1][location_id_2];
    }

    /** Get a location. */
    inline const Location& location(LocationId location_id) const { return locations_[location_id]; }

    /** Get the capacity of the vehicle. */
    inline Demand capacity() const { return locations_[0].demand; }

    /*
     * Outputs
     */

    /** Print the instance. */
    void format(
            std::ostream& os,
            int verbosity_level = 1) const
    {
        if (verbosity_level >= 1) {
            os
                << "Number of locations:  " << number_of_locations() << std::endl
                << "Capacity:             " << capacity() << std::endl
                ;
        }

        if (verbosity_level >= 2) {
            os << std::endl
                << std::setw(12) << "Location"
                << std::setw(12) << "Demand"
                << std::setw(12) << "Serv. time"
                << std::setw(12) << "Rel. date"
                << std::setw(12) << "Deadline"
                << std::setw(12) << "Profit"
                << std::endl
                << std::setw(12) << "--------"
                << std::setw(12) << "----------"
                << std::setw(12) << "------"
                << std::setw(12) << "---------"
                << std::setw(12) << "--------"
                << std::setw(12) << "------"
                << std::endl;
            for (LocationId location_id_1 = 0;
                    location_id_1 < number_of_locations();
                    ++location_id_1) {
                os << std::setw(12) << location_id_1
                    << std::setw(12) << location(location_id_1).demand
                    << std::setw(12) << location(location_id_1).service_time
                    << std::setw(12) << location(location_id_1).release_date
                    << std::setw(12) << location(location_id_1).deadline
                    << std::setw(12) << location(location_id_1).profit
                    << std::endl;
            }
        }

        if (verbosity_level >= 3) {
            os << std::endl
                << std::setw(12) << "Loc. 1"
                << std::setw(12) << "Loc. 2"
                << std::setw(12) << "Tr. time"
                << std::endl
                << std::setw(12) << "------"
                << std::setw(12) << "------"
                << std::setw(12) << "--------"
                << std::endl;
            for (LocationId location_id_1 = 0;
                    location_id_1 < number_of_locations();
                    ++location_id_1) {
                for (LocationId location_id_2 = 0;
                        location_id_2 < number_of_locations();
                        ++location_id_2) {
                    os
                        << std::setw(12) << location_id_1
                        << std::setw(12) << location_id_2
                        << std::setw(12) << travel_time(location_id_1, location_id_2)
                        << std::endl;
                }
            }
        }
    }

private:

    /*
     * Private methods
     */

    /** Create an instance manually. */
    Instance() { }

    /*
     * Private attributes
     */

    /** Locations. */
    std::vector<Location> locations_;

    /** Travel times. */
    std::vector<std::vector<Time>> travel_times_;

    friend class InstanceBuilder;

};

/**
 * Instance builder class for an 'espprctw' problem.
 */
class InstanceBuilder
{

public:

    InstanceBuilder(LocationId number_of_locations)
    {
        instance_.locations_ = std::vector<Location>(number_of_locations);
        instance_.travel_times_ = std::vector<std::vector<Time>>(
                number_of_locations,
                std::vector<Time>(number_of_locations, -1));
        for (LocationId location_id = 0;
                location_id < number_of_locations;
                ++location_id) {
            instance_.travel_times_[location_id][location_id] = std::numeric_limits<Time>::max();
        }
    }

    void set_capacity(Demand demand) { instance_.locations_[0].demand = demand; }

    void set_location_demand(
            LocationId location_id,
            Demand demand)
    {
        instance_.locations_[location_id].demand = demand;
    }

    void set_location_profit(
            LocationId location_id,
            Profit profit)
    {
        instance_.locations_[location_id].profit = profit;
    }

    void set_location_release_date(
            LocationId location_id,
            Time release_date)
    {
        instance_.locations_[location_id].release_date = release_date;
    }

    void set_location_deadline(
            LocationId location_id,
            Time deadline)
    {
        instance_.locations_[location_id].deadline = deadline;
    }

    void set_location_service_time(
            LocationId location_id,
            Time service_time)
    {
        instance_.locations_[location_id].service_time = service_time;
    }

    void set_travel_time(
            LocationId location_id_1,
            LocationId location_id_2,
            Time travel_time)
    {
        instance_.travel_times_[location_id_1][location_id_2] = travel_time;
    }


    /*
     * Build
     */

    /** Build. */
    Instance build()
    {
        return std::move(instance_);
    }

private:

    /*
     * Private attributes
     */

    /** Instance. */
    Instance instance_;

};

class BranchingScheme
{

public:

    struct Node
    {
        std::shared_ptr<Node> parent = nullptr;
        boost::dynamic_bitset<> available_locations;
        LocationId last_location_id = 0;
        LocationId number_of_locations = 1;
        Time cost = 0;
        Time time = 0;
        Profit profit = 0;
        Profit remaining_profit = 0;
        Demand demand = 0;
        Demand remaining_demand = 0;
        double guide = 0;
        LocationPos next_child_pos = 1;

        /** Unique id of the node. */
        treesearchsolver::NodeId id;
    };

    BranchingScheme(const Instance& instance):
        instance_(instance) { }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingScheme::Node());
        r->id = node_id_;
        node_id_++;
        r->available_locations.resize(instance_.number_of_locations(), true);
        r->available_locations[0] = false;
        for (LocationId j = 0; j < instance_.number_of_locations(); ++j) {
            r->remaining_demand += instance_.location(j).demand;
            r->remaining_profit += instance_.location(j).profit;
        }
        return r;
    }

    inline std::shared_ptr<Node> next_child(
            const std::shared_ptr<Node>& parent) const
    {
        assert(!infertile(parent));
        assert(!leaf(parent));
        LocationId next_location_id = parent->next_child_pos;
        const Location& location = instance_.location(next_location_id);
        // Update parent
        parent->next_child_pos++;
        if (!parent->available_locations[next_location_id])
            return nullptr;
        if (parent->demand + location.demand > instance_.capacity())
            return nullptr;
        Time t = instance_.travel_time(parent->last_location_id, next_location_id);
        Time s = std::max(parent->time + t, location.release_date);
        if (s > location.deadline)
            return nullptr;

        // Compute new child.
        auto child = std::shared_ptr<Node>(new BranchingScheme::Node());
        child->id = node_id_;
        node_id_++;
        child->parent = parent;
        child->available_locations = parent->available_locations;
        child->available_locations[next_location_id] = false;
        child->last_location_id = next_location_id;
        child->number_of_locations = parent->number_of_locations + 1;
        child->demand = parent->demand + location.demand;
        child->remaining_demand = parent->remaining_demand - location.demand;
        child->time = s + location.service_time;
        child->cost = parent->cost + t;
        child->profit = parent->profit + location.profit;
        child->remaining_profit = parent->remaining_profit - location.profit;
        for (LocationId j = 0; j < instance_.number_of_locations(); ++j) {
            if (!child->available_locations[j])
                continue;
            if (child->demand + instance_.location(j).demand > instance_.capacity()) {
                child->available_locations[j] = false;
                child->remaining_demand -= instance_.location(j).demand;
                child->remaining_profit -= instance_.location(j).profit;
            } else if (child->time + instance_.travel_time(next_location_id, j) > instance_.location(j).deadline) {
                child->available_locations[j] = false;
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
        return node->next_child_pos == instance_.number_of_locations();
    }

    inline bool operator()(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        assert(!infertile(node_1));
        assert(!infertile(node_2));
        if (node_1->number_of_locations != node_2->number_of_locations)
            return node_1->number_of_locations < node_2->number_of_locations;
        if (node_1->guide != node_2->guide)
            return node_1->guide < node_2->guide;
        return node_1->id < node_2->id;
    }

    inline bool leaf(
            const std::shared_ptr<Node>& node) const
    {
        return node->number_of_locations == instance_.number_of_locations();
    }

    bool bound(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_locations == 1)
            return false;
        return node_1->cost + instance_.travel_time(node_1->last_location_id, 0) - node_1->profit - node_1->remaining_profit
                >= node_2->cost + instance_.travel_time(node_2->last_location_id, 0) - node_2->profit;
    }

    bool better(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        return node_1->cost + instance_.travel_time(node_1->last_location_id, 0) - node_1->profit
                < node_2->cost + instance_.travel_time(node_2->last_location_id, 0) - node_2->profit;
    }

    bool equals(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_locations != node_2->number_of_locations)
            return false;
        std::vector<bool> v(instance_.number_of_locations(), false);
        for (auto node_tmp = node_1;
                node_tmp->parent != nullptr;
                node_tmp = node_tmp->parent) {
            v[node_tmp->last_location_id] = true;
        }
        for (auto node_tmp = node_1;
                node_tmp->parent != nullptr;
                node_tmp = node_tmp->parent) {
            if (!v[node_tmp->last_location_id])
                return false;
        }
        return true;
    }

    std::string display(const std::shared_ptr<Node>& node) const
    {
        if (node->last_location_id == 0)
            return "";
        std::stringstream ss;
        ss << node->cost + instance_.travel_time(node->last_location_id, 0) - node->profit
            << " (n" << node->number_of_locations
            << " c" << node->cost + instance_.travel_time(node->last_location_id, 0)
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
            if (node_1->last_location_id != node_2->last_location_id)
                return false;
            return true;
        }

        inline std::size_t operator()(
                const std::shared_ptr<Node>& node) const
        {
            size_t hash = hasher_1(node->last_location_id);
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
                && (node_1->available_locations | node_2->available_locations)
                == node_1->available_locations)
            return true;
        return false;
    }

private:

    const Instance& instance_;

    mutable treesearchsolver::NodeId node_id_ = 0;

};

}
}
