/**
 * Elementary shortest path problem with resource constraint
 *
 * Input:
 * - n locations with (j = 1..n)
 *   - a demand dⱼ
 *   - a profit pⱼ
 * - an n×n matrix containing the distances between each pair of locations
 * - a capacity c
 * Problem:
 * - find a tour from city 1 to city 1 such that:
 *   - each other location is visited at most once
 *   - the total demand of the visited locations does not exceed the capacity
 * Objective:
 * - minimize the total length of the tour minus the profit a the visited
 *   locations
 *
 * Tree search:
 * - forward branching
 * - guide: current length + distance to the closest next child
 *
 */

#pragma once

#include "optimizationtools/utils/utils.hpp"
#include "optimizationtools/containers//sorted_on_demand_array.hpp"

#include "treesearchsolver/common.hpp"

#include <memory>

namespace columngenerationsolver
{
namespace espprc
{

using LocationId = int64_t;
using LocationPos = int64_t;
using Demand = int64_t;
using Distance = double;
using Profit = double;

/**
 * Structure for a location.
 */
struct Location
{
    /** Demand. */
    Demand demand;

    /** Profit. */
    Profit profit;
};

/**
 * Instance class for an 'espprc' problem.
 */
class Instance
{

public:

    /*
     * Getters
     */

    /** Get the number of locations. */
    inline LocationId number_of_locations() const { return locations_.size(); }

    /** Get the distance between two locations. */
    inline Distance distance(
            LocationId location_id_1,
            LocationId location_id_2) const
    {
        return distances_[location_id_1][location_id_2];
    }

    /** Get a location. */
    inline const Location& location(LocationId location_id) const { return locations_[location_id]; }

    /** Get the capacity. */
    inline Demand capacity() const { return locations_[0].demand; }

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

    /** Distances. */
    std::vector<std::vector<Distance>> distances_;

    friend class InstanceBuilder;

};

/**
 * Instance builder class for an 'espprc' problem.
 */
class InstanceBuilder
{

public:

    /** Constructor. */
    InstanceBuilder(LocationId number_of_locations)
    {
        instance_.locations_ = std::vector<Location>(number_of_locations);
        instance_.distances_ = std::vector<std::vector<Distance>>(
                number_of_locations,
                std::vector<Distance>(number_of_locations, -1));
        for (LocationId location_id = 0;
                location_id < number_of_locations;
                ++location_id) {
            instance_.distances_[location_id][location_id] = std::numeric_limits<Distance>::max();
        }
    }

    /** Set the capacity of the vehicle. */
    void set_capacity(Demand demand) { instance_.locations_[0].demand = demand; }

    /** Set the demand of a location. */
    void set_demand(
            LocationId location_id,
            Demand demand)
    {
        instance_.locations_[location_id].demand = demand;
    }

    /** Set the profit of a location. */
    void set_profit(
            LocationId location_id,
            Profit profit)
    {
        instance_.locations_[location_id].profit = profit;
    }

    /** Set the distance between two locations. */
    void set_distance(
            LocationId location_id_1,
            LocationId location_id_2,
            Distance distance)
    {
        instance_.distances_[location_id_1][location_id_2] = distance;
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
        std::vector<bool> available_locations;
        LocationId last_location_id = 0;
        LocationId number_of_locations = 1;
        Distance length = 0;
        Profit profit = 0;
        Demand demand = 0;
        double guide = 0;
        LocationPos next_child_pos = 0;

        /** Unique id of the node. */
        treesearchsolver::NodeId id;
    };

    BranchingScheme(const Instance& instance):
        instance_(instance),
        sorted_locations_(instance.number_of_locations()),
        generator_(0)
    {
        // Initialize sorted_locations_.
        for (LocationId location_id = 0;
                location_id < instance_.number_of_locations();
                ++location_id) {
            sorted_locations_[location_id].reset(instance.number_of_locations());
            for (LocationId location_id_2 = 0; location_id_2 < instance_.number_of_locations(); ++location_id_2)
                sorted_locations_[location_id].set_cost(location_id_2, instance_.distance(location_id, location_id_2) - instance_.location(location_id_2).profit);
        }
    }

    inline LocationId neighbor(LocationId location_id, LocationPos pos) const
    {
        assert(location_id >= 0);
        assert(location_id < instance_.number_of_locations());
        assert(pos >= 0);
        assert(pos < instance_.number_of_locations());
        return sorted_locations_[location_id].get(pos, generator_);
    }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingScheme::Node());
        r->id = node_id_;
        node_id_++;
        r->available_locations.resize(instance_.number_of_locations(), true);
        r->available_locations[0] = false;
        r->guide = instance_.distance(0, neighbor(0, 0));
        return r;
    }

    inline std::shared_ptr<Node> next_child(
            const std::shared_ptr<Node>& parent) const
    {
        assert(!infertile(parent));
        assert(!leaf(parent));
        LocationId next_location_id = neighbor(parent->last_location_id, parent->next_child_pos);
        Distance d = instance_.distance(parent->last_location_id, next_location_id);
        // Update parent
        parent->next_child_pos++;
        LocationId next_location_id_next = neighbor(parent->last_location_id, parent->next_child_pos);
        Distance d_next = instance_.distance(parent->last_location_id, next_location_id_next);
        if (d_next == std::numeric_limits<Distance>::max()) {
            parent->guide = std::numeric_limits<double>::max();
        } else {
            parent->guide = parent->length + d_next
                - parent->profit - instance_.location(next_location_id_next).profit;
        }
        if (parent->demand + instance_.location(next_location_id).demand > instance_.capacity())
            return nullptr;
        if (!parent->available_locations[next_location_id])
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
        child->length = parent->length + d;
        child->profit = parent->profit + instance_.location(next_location_id).profit;
        child->demand = parent->demand + instance_.location(next_location_id).demand;
        child->guide = child->length + instance_.distance(next_location_id, neighbor(next_location_id, 0))
            - child->profit - instance_.location(neighbor(next_location_id, 0)).profit;
        return child;
    }

    inline bool infertile(
            const std::shared_ptr<Node>& node) const
    {
        assert(node != nullptr);
        return (node->guide == std::numeric_limits<double>::max());
    }

    inline bool operator()(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        assert(!infertile(node_1));
        assert(!infertile(node_2));
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
        (void)node_1;
        (void)node_2;
        return false;
    }

    bool better(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        return node_1->length + instance_.distance(node_1->last_location_id, 0) - node_1->profit
                < node_2->length + instance_.distance(node_2->last_location_id, 0) - node_2->profit;
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
        ss << node->length + instance_.distance(node->last_location_id, 0) - node->profit
            << " (n" << node->number_of_locations
            << " l" << node->length + instance_.distance(node->last_location_id, 0)
            << " p" << node->profit
            << ")";
        return ss.str();
    }

    /**
     * Dominances
     */

    inline bool comparable(
            const std::shared_ptr<Node>&) const
    {
        return true;
    }

    struct NodeHasher
    {
        std::hash<LocationId> hasher_1;
        std::hash<std::vector<bool>> hasher_2;

        inline bool operator()(
                const std::shared_ptr<Node>& node_1,
                const std::shared_ptr<Node>& node_2) const
        {
            if (node_1->last_location_id != node_2->last_location_id)
                return false;
            if (node_1->available_locations != node_2->available_locations)
                return false;
            return true;
        }

        inline std::size_t operator()(
                const std::shared_ptr<Node>& node) const
        {
            size_t hash = hasher_1(node->last_location_id);
            optimizationtools::hash_combine(hash, hasher_2(node->available_locations));
            return hash;
        }
    };

    inline NodeHasher node_hasher() const { return NodeHasher(); }

    inline bool dominates(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->length - node_1->profit <= node_2->length - node_2->profit
                && node_1->demand <= node_2->demand)
            return true;
        return false;
    }

private:

    const Instance& instance_;

    mutable std::vector<optimizationtools::SortedOnDemandArray> sorted_locations_;

    mutable std::mt19937_64 generator_;

    mutable treesearchsolver::NodeId node_id_ = 0;

};

}
}
