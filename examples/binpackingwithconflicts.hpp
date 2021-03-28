#pragma once

#include "columngenerationsolver/commons.hpp"

#include "examples/knapsackwithconflicts.hpp"
#include "treesearchsolver/algorithms/iterative_beam_search.hpp"

/**
 * Bin Packing Problem with Conflicts.
 *
 * Input:
 * - a capacity c
 * - n items with weight wⱼ (j = 1..n)
 * - a graph G such that each node corresponds to an item
 * Problem:
 * - Pack all items into bins such that:
 *   - the total weight of the items of a bin does not exceed the capacity c
 *   - if there exists an edge between j₁ to j₂ in G, then j₁ and j₂ must not
 *     be in the same bin
 * Objective:
 * - Minimize the number of bin used.
 *
 * The linear programming formulation of the problem based on Dantzig–Wolfe
 * decomposition is written as follows:
 *
 * Variables:
 * - yᵏ ∈ {0, 1} representing a set of items fitting into a bin.
 *   yᵏ = 1 iff the corresponding set of items is selected.
 *   xⱼᵏ = 1 iff yᵏ contains item j, otherwise 0.
 *
 * Program:
 *
 * min ∑ₖ yᵏ
 *
 * 1 <= ∑ₖ xⱼᵏ yᵏ <= 1       for all items j
 *                                                 (each item must be packed)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of negative reduced cost.
 * The reduced cost of a variable yᵏ is given by:
 * rc(yᵏ) = 1 - ∑ⱼ xⱼᵏ vⱼ
 *        = - ∑ⱼ vⱼ xⱼᵏ + 1
 *
 * Therefore, finding a variable of minium reduced cost reduces to solving
 * a Knapsack Problem with Conflicts with items with profit vⱼ.
 *
 */


namespace columngenerationsolver
{

namespace binpackingwithconflictssolver
{

typedef int64_t ItemId;
typedef int64_t Weight;
typedef int64_t BinId;

struct Item
{
    ItemId id;
    Weight weight;
    std::vector<ItemId> neighbors;
};

class Instance
{

public:

    Instance() { }
    void set_capacity(Weight capacity) { capacity_ = capacity; }
    void add_item(Weight weight)
    {
        Item item;
        item.id = items_.size();
        item.weight = weight;
        items_.push_back(item);
    }
    void add_conflict(ItemId j1, ItemId j2)
    {
        assert(j1 >= 0);
        assert(j2 >= 0);
        assert(j1 < item_number());
        assert(j2 < item_number());
        items_[j1].neighbors.push_back(j2);
        items_[j2].neighbors.push_back(j1);
    }

    Instance(std::string instance_path, std::string format = "")
    {
        std::ifstream file(instance_path);
        if (!file.good()) {
            std::cerr << "\033[31m" << "ERROR, unable to open file \"" << instance_path << "\"" << "\033[0m" << std::endl;
            assert(false);
            return;
        }
        if (format == "" || format == "default") {
            read_default(file);
        } else {
            std::cerr << "\033[31m" << "ERROR, unknown instance format \"" << format << "\"" << "\033[0m" << std::endl;
        }
        file.close();
    }

    virtual ~Instance() { }

    ItemId item_number() const { return items_.size(); }
    Weight capacity() const { return capacity_; }
    const Item& item(ItemId j) const { return items_[j]; }

private:

    void read_default(std::ifstream& file)
    {
        std::string tmp;
        std::vector<std::string> line;

        ItemId n;
        Weight c;
        file >> n >> c;
        set_capacity(c);

        Weight w = -1;
        ItemId j_tmp = -1;
        for (ItemId j = 0; j < n; ++j) {
            file >> j_tmp >> w;
            add_item(w);
            getline(file, tmp);
            line = optimizationtools::split(tmp, ' ');
            for (std::string s: line)
                if (std::stol(s) - 1 < j)
                    add_conflict(j, std::stol(s) - 1);
        }
    }

    std::vector<Item> items_;
    Weight capacity_;

};

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        packed_items_(instance.item_number()),
        bpp2kp_(instance.item_number())
    { }

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<int8_t> packed_items_;

    std::vector<ItemId> kp2bpp_;
    std::vector<treesearchsolver::knapsackwithconflicts::ItemId> bpp2kp_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    ItemId n = instance.item_number();
    columngenerationsolver::Parameters p(n);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Min;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row bounds.
    for (ItemId j = 0; j < n; ++j) {
        p.row_lower_bounds[j] = 1;
        p.row_upper_bounds[j] = 1;
        p.row_coefficient_lower_bounds[j] = 0;
        p.row_coefficient_upper_bounds[j] = 1;
    }
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = 2 * n;
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));
    return p;
}

std::vector<ColIdx> PricingSolver::initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns)
{
    std::fill(packed_items_.begin(), packed_items_.end(), 0);
    for (auto p: fixed_columns) {
        const Column& column = columns[p.first];
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
            RowIdx row_index = column.row_indices[row_pos];
            Value row_coefficient = column.row_coefficients[row_pos];
            packed_items_[row_index] += value * row_coefficient;
        }
    }
    return {};
}

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    ItemId n = instance_.item_number();

    // Build subproblem instance.
    treesearchsolver::knapsackwithconflicts::Instance instance_kp;
    instance_kp.set_capacity(instance_.capacity());
    kp2bpp_.clear();
    std::fill(bpp2kp_.begin(), bpp2kp_.end(), -1);
    for (ItemId j = 0; j < n; ++j) {
        treesearchsolver::knapsackwithconflicts::Profit profit = duals[j];
        if (profit <= 0)
            continue;
        if (packed_items_[j] == 1)
            continue;
        bpp2kp_[j] = kp2bpp_.size();
        kp2bpp_.push_back(j);
        instance_kp.add_item(instance_.item(j).weight, duals[j]);
        for (ItemId j2: instance_.item(j).neighbors)
            if (j2 < j && bpp2kp_[j2] != -1)
                instance_kp.add_conflict(bpp2kp_[j], bpp2kp_[j2]);
    }
    ItemId n_kp = kp2bpp_.size();

    // Solve subproblem instance.
    treesearchsolver::knapsackwithconflicts::BranchingScheme branching_scheme(instance_kp, {});
    treesearchsolver::IterativeBeamSearchOptionalParameters parameters_kp;
    parameters_kp.solution_pool_size_max = 100;
    parameters_kp.queue_size_min = 512;
    parameters_kp.queue_size_max = 512;
    //parameters_espp.info.set_verbose(true);
    auto output_kp = treesearchsolver::iterativebeamsearch(
            branching_scheme, parameters_kp);

    // Retrieve column.
    std::vector<Column> columns;
    ItemId i = 0;
    for (const std::shared_ptr<treesearchsolver::knapsackwithconflicts::BranchingScheme::Node>& node:
            output_kp.solution_pool.solutions()) {
        if (i > 2 * n_kp)
            break;
        Column column;
        column.objective_coefficient = 1;
        for (auto node_tmp = node; node_tmp->father != nullptr; node_tmp = node_tmp->father) {
            i++;
            column.row_indices.push_back(kp2bpp_[node_tmp->j]);
            column.row_coefficients.push_back(1);
        }
        columns.push_back(column);
    }
    return columns;
}

}

}
