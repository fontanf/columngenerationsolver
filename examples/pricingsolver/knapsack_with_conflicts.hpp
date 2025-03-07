/**
 * Knapsack problem with conflicts
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/include/orproblems/packing/knapsack_with_conflicts.hpp
 *
 * Branching scheme:
 * - Root node: empty solution, no item
 * - Children: add a new item in the knapsack, i.e. create one child for each
 *   valid item.
 * - Dominance: if two nodes node_1 and node_2 have the same available items
 *   left and:
 *   - profit(node_1) >= profit(node_2)
 *   - weight(node_1) <= weight(node_2)
 *   then node_1 dominates node_2
 *
 */

#pragma once

#include "orproblems/packing/knapsack_with_conflicts.hpp"

#include <memory>
#include <sstream>

namespace columngenerationsolver
{
namespace knapsack_with_conflicts
{

using namespace orproblems::knapsack_with_conflicts;

using NodeId = int64_t;
using GuideId = int64_t;

class BranchingScheme
{

public:

    struct Parameters
    {
        GuideId guide_id = 0;
    };

    struct Node
    {
        /** Parent node. */
        std::shared_ptr<Node> parent = nullptr;

        /** Array indicating for each item, if it still available. */
        std::vector<bool> available_items;

        /** Last item added to the partial solution. */
        ItemId item_id = -1;

        /** Number of items in the partial solution. */
        ItemId number_of_items = 0;

        /** Number of remaining available items. */
        ItemId number_of_remaining_items = -1;

        /** Weight of the remaining available items. */
        Profit remaining_weight = 0;

        /** Profit of the remaining available items. */
        Profit remaining_profit = 0;

        /** Weight of the partial solution. */
        Weight weight = 0;

        /** Profit of the partial solution. */
        Profit profit = 0;

        /** Guide. */
        double guide = 0;

        /** Next child to generate. */
        ItemPos next_child_pos = 0;

        /** Unique id of the node. */
        NodeId id = -1;
    };

    BranchingScheme(
            const Instance& instance,
            const Parameters& parameters):
        instance_(instance),
        parameters_(parameters) { }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingScheme::Node());
        r->id = node_id_;
        node_id_++;
        r->available_items.resize(instance_.number_of_items(), true);
        r->number_of_remaining_items = instance_.number_of_items();
        for (ItemId item_id = 0;
                item_id < instance_.number_of_items();
                ++item_id) {
            r->remaining_weight += instance_.item(item_id).weight;
            r->remaining_profit += instance_.item(item_id).profit;
        }
        return r;
    }

    inline std::shared_ptr<Node> next_child(
            const std::shared_ptr<Node>& parent) const
    {
        // Get the next item to add.
        ItemId item_id_next = parent->next_child_pos;

        // Update parent
        parent->next_child_pos++;

        // Check if the item is still available.
        if (!parent->available_items[item_id_next])
            return nullptr;

        // Check if the item fit in the knapsack.
        if (parent->weight + instance_.item(item_id_next).weight > instance_.capacity())
            return nullptr;

        // Compute new child.
        auto child = std::shared_ptr<Node>(new BranchingScheme::Node());
        child->id = node_id_;
        node_id_++;
        child->parent = parent;
        child->item_id = item_id_next;
        child->number_of_items = parent->number_of_items + 1;
        child->available_items = parent->available_items;
        child->available_items[item_id_next] = false;
        child->number_of_remaining_items = parent->number_of_remaining_items - 1;
        child->remaining_weight = parent->remaining_weight - instance_.item(item_id_next).weight;
        child->remaining_profit = parent->remaining_profit - instance_.item(item_id_next).profit;
        for (ItemId item_id: instance_.item(item_id_next).neighbors) {
            if (child->available_items[item_id]) {
                child->available_items[item_id] = false;
                child->number_of_remaining_items--;
                child->remaining_weight -= instance_.item(item_id).weight;
                child->remaining_profit -= instance_.item(item_id).profit;
            }
        }
        child->weight = parent->weight + instance_.item(item_id_next).weight;
        child->profit = parent->profit + instance_.item(item_id_next).profit;
        child->guide =
            (parameters_.guide_id == 0)? (double)child->weight / child->profit:
            (parameters_.guide_id == 1)? (double)child->weight / child->profit / child->remaining_profit:
                                         (double)1.0 / (child->profit + child->remaining_profit);
        return child;
    }

    inline bool infertile(
            const std::shared_ptr<Node>& node) const
    {
        return (node->next_child_pos == instance_.number_of_items());
    }

    inline bool operator()(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_items != node_2->number_of_items)
            return node_1->number_of_items < node_2->number_of_items;
        if (node_1->guide != node_2->guide)
            return node_1->guide < node_2->guide;
        return node_1->id < node_2->id;
    }

    inline bool leaf(
            const std::shared_ptr<Node>& node) const
    {
        return node->number_of_items == instance_.number_of_items();
    }

    bool bound(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        return node_1->profit + node_1->remaining_profit <= node_2->profit;
    }

    /*
     * Solution pool.
     */

    bool better(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        return node_1->profit > node_2->profit;
    }

    std::shared_ptr<Node> goal_node(double value) const
    {
        auto node = std::shared_ptr<Node>(new BranchingScheme::Node());
        node->profit = value;
        return node;
    }

    bool equals(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_items != node_2->number_of_items)
            return false;
        std::vector<bool> v(instance_.number_of_items(), false);
        for (auto node_tmp = node_1; node_tmp->parent != nullptr; node_tmp = node_tmp->parent)
            v[node_tmp->item_id] = true;
        for (auto node_tmp = node_1; node_tmp->parent != nullptr; node_tmp = node_tmp->parent)
            if (!v[node_tmp->item_id])
                return false;
        return true;
    }

    std::string display(const std::shared_ptr<Node>& node) const
    {
        std::stringstream ss;
        ss << node->profit
            << " (n" << node->number_of_items << "/" << instance_.number_of_items()
            << " w" << node->weight << "/" << instance_.capacity()
            << ")";
        return ss.str();
    }

    /*
     * Dominances.
     */

    inline bool comparable(
            const std::shared_ptr<Node>&) const
    {
        return true;
    }

    struct NodeHasher
    {
        std::hash<std::vector<bool>> hasher;

        inline bool operator()(
                const std::shared_ptr<Node>& node_1,
                const std::shared_ptr<Node>& node_2) const
        {
            return node_1->available_items == node_2->available_items;
        }

        inline std::size_t operator()(
                const std::shared_ptr<Node>& node) const
        {
            size_t hash = hasher(node->available_items);
            return hash;
        }
    };

    inline NodeHasher node_hasher() const { return NodeHasher(); }

    inline bool dominates(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->profit >= node_2->profit
                && node_1->weight <= node_2->weight)
            return true;
        return false;
    }

    /*
     * Outputs
     */

    void instance_format(
            std::ostream& os,
            int verbosity_level) const
    {
        instance_.format(os, verbosity_level);
    }

    void solution_format(
            const std::shared_ptr<Node>& node,
            std::ostream& os,
            int verbosity_level) const
    {
        if (verbosity_level >= 1) {
            os
                << "Profit:           " << node->profit << std::endl
                << "Weight:           " << node->weight << " / " << instance_.capacity() << std::endl
                << "Number of items:  " << node->number_of_items << " / " << instance_.number_of_items() << std::endl
                ;
        }
        if (verbosity_level >= 2) {
            os << std::endl
                << std::setw(12) << "Item"
                << std::setw(12) << "Profit"
                << std::setw(12) << "Weight"
                << std::setw(12) << "Efficiency"
                << std::setw(12) << "# conflicts"
                << std::endl
                << std::setw(12) << "----"
                << std::setw(12) << "------"
                << std::setw(12) << "------"
                << std::setw(12) << "----------"
                << std::setw(12) << "-----------"
                << std::endl;
            for (auto node_tmp = node;
                    node_tmp->parent != nullptr;
                    node_tmp = node_tmp->parent) {
                ItemId item_id = node_tmp->item_id;
                const Item& item = instance_.item(item_id);
                os
                    << std::setw(12) << item_id
                    << std::setw(12) << item.profit
                    << std::setw(12) << item.weight
                    << std::setw(12) << (double)item.profit / item.weight
                    << std::setw(12) << item.neighbors.size()
                    << std::endl;
            }
        }
    }

    inline void solution_write(
            const std::shared_ptr<Node>& node,
            std::string certificate_path) const
    {
        if (certificate_path.empty())
            return;
        std::ofstream file(certificate_path);
        if (!file.good()) {
            throw std::runtime_error(
                    "Unable to open file \"" + certificate_path + "\".");
        }

        std::vector<ItemId> items;
        for (auto node_tmp = node; node_tmp->parent != nullptr;
                node_tmp = node_tmp->parent)
            items.push_back(node_tmp->item_id);
        std::reverse(items.begin(), items.end());
        for (ItemId item_id: items)
            file << item_id << " ";
    }

private:

    /** Instance. */
    const Instance& instance_;

    /** Parameters. */
    Parameters parameters_;

    mutable NodeId node_id_ = 0;

};

}
}
