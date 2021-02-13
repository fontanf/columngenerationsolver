#pragma once

#include "optimizationtools/info.hpp"
#include "optimizationtools/utils.hpp"
#include "optimizationtools/sorted_on_demand_array.hpp"

namespace columngenerationsolver
{

namespace espprc
{

typedef int64_t VertexId;
typedef int64_t VertexPos;
typedef int64_t Demand;
typedef double Distance;

struct Location
{
    Demand demand;
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
    void set_demand(VertexId j, Demand demand) { locations_[j].demand = demand; }
    void set_distance(VertexId j1, VertexId j2, Distance d)
    {
        assert(j1 >= 0);
        assert(j2 >= 0);
        assert(j1 < vertex_number());
        assert(j2 < vertex_number());
        distances_[j1][j2] = d;
        distance_max_ = std::max(distance_max_, d);
    }

    virtual ~Instance() { }

    inline VertexId vertex_number() const { return locations_.size(); }
    inline Distance distance(VertexId j1, VertexId j2) const { return distances_[j1][j2]; }
    inline Demand demand(VertexId j) const { return locations_[j].demand; }
    inline Demand capacity() const { return locations_[0].demand; }
    inline Distance maximum_distance() const { return distance_max_; }

private:

    std::vector<Location> locations_;
    std::vector<std::vector<Distance>> distances_;
    Distance distance_max_ = 0;

};

class BranchingScheme
{

public:

    struct Node
    {
        std::shared_ptr<Node> father = nullptr;
        std::vector<bool> visited; // All visited vertices but the last.
        VertexId j = 0; // Last visited vertex.
        VertexId vertex_number = 1;
        Distance length = 0;
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
                sorted_vertices_[j].set_cost(j2, instance_.distance(j, j2));
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
        r->visited.resize(instance_.vertex_number(), false);
        r->guide = instance_.distance(0, neighbor(0, 0));
        return r;
    }

    inline std::shared_ptr<Node> next_child(
            const std::shared_ptr<Node> father) const
    {
        if (infertile(father))
            std::cout << father->j << " " << father->next_child_pos
                << " " << instance_.distance(0, 0)
                << " " << instance_.distance(0, 1)
                << std::endl;
        assert(!infertile(father));
        assert(!leaf(father));

        VertexId j_next = neighbor(father->j, father->next_child_pos);
        Distance d = instance_.distance(father->j, j_next);
        // Update father
        father->next_child_pos++;
        Distance d_next = instance_.distance(
                father->j,
                neighbor(father->j, father->next_child_pos));
        if (d_next == std::numeric_limits<Distance>::max()) {
            father->guide = std::numeric_limits<double>::max();
        } else {
            father->guide = father->guide - d + d_next;
        }
        if (father->demand + instance_.demand(j_next) > instance_.capacity())
            return nullptr;
        if (father->visited[j_next])
            return nullptr;

        // Compute new child.
        auto child = std::shared_ptr<Node>(new BranchingScheme::Node());
        child->father = father;
        child->visited = father->visited;
        child->visited[father->j] = true;
        child->j = j_next;
        child->vertex_number = father->vertex_number + 1;
        child->length = father->length + d;
        child->demand = father->demand + instance_.demand(j_next);
        child->guide = child->length
            + instance_.distance(j_next, neighbor(j_next, 0));
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
        return node_1->length + instance_.distance(node_1->j, 0)
            < node_2->length + instance_.distance(node_2->j, 0);
    }

    bool equals(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        return node_1->visited == node_2->visited;
    }

    std::string display(const std::shared_ptr<Node>& node) const
    {
        if (node->j == 0)
            return "";
        return std::to_string(node->length + instance_.distance(node->j, 0));
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
            return node_1->visited == node_2->visited;
        }

        inline std::size_t operator()(
                const std::shared_ptr<Node>& node) const
        {
            size_t hash = hasher_1(node->j);
            optimizationtools::hash_combine(hash, hasher_2(node->visited));
            return hash;
        }
    };

    inline NodeHasher node_hasher() const { return NodeHasher(); }

    inline bool dominates(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->length > node_2->length)
            return false;
        return true;
    }

private:

    const Instance& instance_;

    mutable std::vector<optimizationtools::SortedOnDemandArray> sorted_vertices_;
    mutable std::mt19937_64 generator_;

};

}

}

