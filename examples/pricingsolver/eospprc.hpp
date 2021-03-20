#pragma once

#include "optimizationtools/info.hpp"
#include "optimizationtools/utils.hpp"
#include "optimizationtools/sorted_on_demand_array.hpp"

/**
 * Elementary Open Shortest Path Problem with Resource Constraint.
 *
 * Input:
 * - n vertices with (j = 1..n)
 *   - a demand dⱼ
 *   - a profit pⱼ
 * - an n×n matrix containing the distances between each pair of vertices
 * - A capacity Q
 * - A maximum route length L
 * Problem:
 * - find a path from city 1 to any city such that:
 *   - each other vertex is visited at most once
 *   - the total demand of the visited vertices does not exceed the capacity
 *   - the total length of the path does not exceed the maximum route length
 * Objective:
 * - minimize the total length of the tour minus the profit a the visited
 *   vertices
 *
 * Tree search:
 * - forward branching
 * - guide: current length + distance to the closest next child
 *
 */

namespace columngenerationsolver
{

namespace eospprc
{

typedef int64_t VertexId;
typedef int64_t VertexPos;
typedef int64_t Demand;
typedef double Distance;
typedef double Profit;

struct Location
{
    Demand demand;
    Profit profit;
};

class Instance
{

public:

    Instance(VertexId n):
        locations_(n),
        distances_(n, std::vector<Distance>(n, -1))
    {
        for (VertexId j = 0; j < n; ++j)
            distances_[j][j] = std::numeric_limits<Distance>::max();
    }
    void set_maximum_route_length(Distance maximum_route_length) { maximum_route_length_ = maximum_route_length; }
    void set_capacity(Demand demand) { locations_[0].demand = demand; }
    void set_location(
            VertexId j,
            Demand demand,
            Profit profit)
    {
        locations_[j].demand = demand;
        locations_[j].profit = profit;
    }
    void set_distance(VertexId j1, VertexId j2, Distance d)
    {
        assert(j1 >= 0);
        assert(j2 >= 0);
        assert(j1 < vertex_number());
        assert(j2 < vertex_number());
        distances_[j1][j2] = d;
    }

    virtual ~Instance() { }

    inline VertexId vertex_number() const { return locations_.size(); }
    Distance maximum_route_length() const { return maximum_route_length_; }
    inline Demand capacity() const { return locations_[0].demand; }
    inline const Location& location(VertexId j) const { return locations_[j]; }
    inline Distance distance(VertexId j1, VertexId j2) const { return distances_[j1][j2]; }

private:

    std::vector<Location> locations_;
    std::vector<std::vector<Distance>> distances_;
    Distance maximum_route_length_ = std::numeric_limits<Distance>::infinity();;

};

class BranchingScheme
{

public:

    struct Node
    {
        std::shared_ptr<Node> father = nullptr;
        std::vector<bool> available_vertices;
        VertexId j = 0; // Last visited vertex.
        VertexId vertex_number = 1;
        Distance length = 0;
        Profit profit = 0;
        Demand demand = 0;
        double guide = 0;
        VertexPos next_child_pos = 0;
    };

    BranchingScheme(const Instance& instance):
        instance_(instance),
        sorted_vertices_(instance.vertex_number()),
        generator_(0)
    {
        // Initialize sorted_vertices_.
        for (VertexId j = 0; j < instance_.vertex_number(); ++j) {
            sorted_vertices_[j].reset(instance.vertex_number());
            for (VertexId j2 = 0; j2 < instance_.vertex_number(); ++j2)
                sorted_vertices_[j].set_cost(j2, instance_.distance(j, j2) - instance_.location(j2).profit);
        }
    }

    inline VertexId neighbor(VertexId j, VertexPos pos) const
    {
        assert(j >= 0);
        assert(j < instance_.vertex_number());
        assert(pos >= 0);
        assert(pos < instance_.vertex_number());
        return sorted_vertices_[j].get(pos, generator_);
    }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingScheme::Node());
        r->available_vertices.resize(instance_.vertex_number(), true);
        r->available_vertices[0] = false;
        r->guide = instance_.distance(0, neighbor(0, 0));
        return r;
    }

    inline std::shared_ptr<Node> next_child(
            const std::shared_ptr<Node>& father) const
    {
        assert(!infertile(father));
        assert(!leaf(father));
        VertexId j_next = neighbor(father->j, father->next_child_pos);
        Distance d = instance_.distance(father->j, j_next);
        // Update father
        father->next_child_pos++;
        VertexId j_next_next = neighbor(father->j, father->next_child_pos);
        Distance d_next = instance_.distance(father->j, j_next_next);
        if (d_next == std::numeric_limits<Distance>::max()) {
            father->guide = std::numeric_limits<double>::max();
        } else {
            father->guide = father->length + d_next
                - father->profit - instance_.location(j_next_next).profit;
        }
        if (father->demand + instance_.location(j_next).demand > instance_.capacity())
            return nullptr;
        if (father->length + d > instance_.maximum_route_length())
            return nullptr;
        if (!father->available_vertices[j_next])
            return nullptr;

        // Compute new child.
        auto child = std::shared_ptr<Node>(new BranchingScheme::Node());
        child->father = father;
        child->available_vertices = father->available_vertices;
        child->available_vertices[j_next] = false;
        child->j = j_next;
        child->vertex_number = father->vertex_number + 1;
        child->length = father->length + d;
        child->profit = father->profit + instance_.location(j_next).profit;
        child->demand = father->demand + instance_.location(j_next).demand;
        child->guide = child->length + instance_.distance(j_next, neighbor(j_next, 0))
            - child->profit - instance_.location(neighbor(j_next, 0)).profit;
        //for (VertexId j = 0; j < instance_.vertex_number(); ++j) {
        //    if (!child->available_vertices[j])
        //        continue;
        //    if (child->demand + instance_.location(j).demand > instance_.capacity())
        //        child->available_vertices[j] = false;
        //}
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
        return node_1.get() < node_2.get();
    }

    inline bool leaf(
            const std::shared_ptr<Node>& node) const
    {
        return node->vertex_number == instance_.vertex_number();
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
        return node_1->length - node_1->profit
            < node_2->length - node_2->profit;
    }

    bool equals(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->vertex_number != node_2->vertex_number)
            return false;
        std::vector<bool> v(instance_.vertex_number(), false);
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
        ss << node->length - node->profit
            << " (n" << node->vertex_number
            << " l" << node->length
            << " p" << node->profit
            << ")";
        return ss.str();
    }

    /**
     * Dominances.
     */

    inline bool comparable(
            const std::shared_ptr<Node>& node) const
    {
        (void)node;
        return true;
    }

    struct NodeHasher
    {
        std::hash<VertexId> hasher_1;
        std::hash<std::vector<bool>> hasher_2;

        inline bool operator()(
                const std::shared_ptr<Node>& node_1,
                const std::shared_ptr<Node>& node_2) const
        {
            if (node_1->j != node_2->j)
                return false;
            if (node_1->available_vertices != node_2->available_vertices)
                return false;
            return true;
        }

        inline std::size_t operator()(
                const std::shared_ptr<Node>& node) const
        {
            size_t hash = hasher_1(node->j);
            optimizationtools::hash_combine(hash, hasher_2(node->available_vertices));
            return hash;
        }
    };

    inline NodeHasher node_hasher() const { return NodeHasher(); }

    inline bool dominates(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->length - node_1->profit <= node_2->length - node_2->profit
                && node_1->length <= node_2->length
                && node_1->demand <= node_2->demand)
            return true;
        return false;
    }

private:

    const Instance& instance_;

    mutable std::vector<optimizationtools::SortedOnDemandArray> sorted_vertices_;
    mutable std::mt19937_64 generator_;

};

}

}

