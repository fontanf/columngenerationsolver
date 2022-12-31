/**
 * Elementary Shortest Path Problem with Resource Constraint.
 *
 * Input:
 * - n vertices with (j = 1..n)
 *   - a demand dⱼ
 *   - a profit pⱼ
 * - an n×n matrix containing the distances between each pair of vertices
 * - a capacity c
 * Problem:
 * - find a tour from city 1 to city 1 such that:
 *   - each other vertex is visited at most once
 *   - the total demand of the visited vertices does not exceed the capacity
 * Objective:
 * - minimize the total length of the tour minus the profit a the visited
 *   vertices
 *
 * Tree search:
 * - forward branching
 * - guide: current length + distance to the closest next child
 *
 */

#pragma once

#include "optimizationtools/utils/info.hpp"
#include "optimizationtools/utils/utils.hpp"
#include "optimizationtools/containers/sorted_on_demand_array.hpp"

namespace columngenerationsolver
{

namespace espprc
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
        assert(j1 < number_of_vertices());
        assert(j2 < number_of_vertices());
        distances_[j1][j2] = d;
    }

    virtual ~Instance() { }

    inline VertexId number_of_vertices() const { return locations_.size(); }
    inline Distance distance(VertexId j1, VertexId j2) const { return distances_[j1][j2]; }
    inline const Location& location(VertexId j) const { return locations_[j]; }
    inline Demand capacity() const { return locations_[0].demand; }

private:

    std::vector<Location> locations_;
    std::vector<std::vector<Distance>> distances_;

};

class BranchingScheme
{

public:

    struct Node
    {
        std::shared_ptr<Node> father = nullptr;
        std::vector<bool> available_vertices;
        VertexId j = 0; // Last visited vertex.
        VertexId number_of_vertices = 1;
        Distance length = 0;
        Profit profit = 0;
        Demand demand = 0;
        double guide = 0;
        VertexPos next_child_pos = 0;
    };

    BranchingScheme(const Instance& instance):
        instance_(instance),
        sorted_vertices_(instance.number_of_vertices()),
        generator_(0)
    {
        // Initialize sorted_vertices_.
        for (VertexId j = 0; j < instance_.number_of_vertices(); ++j) {
            sorted_vertices_[j].reset(instance.number_of_vertices());
            for (VertexId j2 = 0; j2 < instance_.number_of_vertices(); ++j2)
                sorted_vertices_[j].set_cost(j2, instance_.distance(j, j2) - instance_.location(j2).profit);
        }
    }

    inline VertexId neighbor(VertexId j, VertexPos pos) const
    {
        assert(j >= 0);
        assert(j < instance_.number_of_vertices());
        assert(pos >= 0);
        assert(pos < instance_.number_of_vertices());
        return sorted_vertices_[j].get(pos, generator_);
    }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingScheme::Node());
        r->available_vertices.resize(instance_.number_of_vertices(), true);
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
        if (!father->available_vertices[j_next])
            return nullptr;

        // Compute new child.
        auto child = std::shared_ptr<Node>(new BranchingScheme::Node());
        child->father = father;
        child->available_vertices = father->available_vertices;
        child->available_vertices[j_next] = false;
        child->j = j_next;
        child->number_of_vertices = father->number_of_vertices + 1;
        child->length = father->length + d;
        child->profit = father->profit + instance_.location(j_next).profit;
        child->demand = father->demand + instance_.location(j_next).demand;
        child->guide = child->length + instance_.distance(j_next, neighbor(j_next, 0))
            - child->profit - instance_.location(neighbor(j_next, 0)).profit;
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
        return node->number_of_vertices == instance_.number_of_vertices();
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
        return node_1->length + instance_.distance(node_1->j, 0) - node_1->profit
            < node_2->length + instance_.distance(node_2->j, 0) - node_2->profit;
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
        ss << node->length + instance_.distance(node->j, 0) - node->profit
            << " (n" << node->number_of_vertices
            << " l" << node->length + instance_.distance(node->j, 0)
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

