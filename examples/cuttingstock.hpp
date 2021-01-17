#pragma once

#include "columngenerationsolver/commons.hpp"

#include "knapsacksolver/algorithms/minknap.hpp"

/**
 * Cutting Stock Problem.
 *
 * Input:
 * - a capacity c
 * - n items with weight wⱼ and demand qⱼ (j = 1..n)
 * Problem:
 * - pack all items such that the total weight of the items in a bin does not
 *   exceed the capacity.
 * Objective:
 * - minimize the number of bin used.
 *
 * The linear programming formulation of the problem based on Dantzig–Wolfe
 * decomposition is written as follows:
 *
 * Variables:
 * - yᵏ ∈ {0, qmax} representing a set of items fitting into a bin.
 *   yᵏ = q iff the corresponding set of items is selected q times.
 *   xⱼᵏ = q iff yᵏ contains q copies of item type j, otherwise 0.
 *
 * Program:
 *
 * min ∑ₖ yᵏ
 *
 * qⱼ <= ∑ₖ xⱼᵏ yᵏ <= qⱼ     for all items j
 *                                      (each item selected exactly qⱼ times)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of negative reduced cost.
 * The reduced cost of a variable yᵏ is given by:
 * rc(yᵏ) = 1 - ∑ⱼ xⱼᵏ vⱼ
 *        = - ∑ⱼ vⱼ xⱼᵏ + 1
 *
 * Therefore, finding a variable of minium reduced cost reduces to solving
 * a Bounded Knapsack Problems with items with profit vⱼ.
 *
 */


namespace columngenerationsolver
{

namespace cuttingstocksolver
{

typedef int64_t ItemTypeId;
typedef int64_t Weight;
typedef int64_t Demand;
typedef int64_t BinId;

struct Item
{
    Weight weight;
    Demand demand;
};

class Instance
{

public:

    Instance() { }
    void set_capacity(Weight capacity) { capacity_ = capacity; }
    void add_item_type(Weight weight, Demand demand = 1)
    {
        item_types_.push_back(Item{weight, demand});
        demand_max_ = std::max(demand_max_, demand);
        demand_sum_ += demand;
    }

    Instance(std::string instance_path, std::string format = "")
    {
        std::ifstream file(instance_path);
        if (!file.good()) {
            std::cerr << "\033[31m" << "ERROR, unable to open file \"" << instance_path << "\"" << "\033[0m" << std::endl;
            assert(false);
            return;
        }
        if (format == "" || format == "bpplib_bpp") {
            read_bpplib_bpp(file);
        } else if (format == "bpplib_csp") {
            read_bpplib_csp(file);
        } else {
            std::cerr << "\033[31m" << "ERROR, unknown instance format \"" << format << "\"" << "\033[0m" << std::endl;
        }
        file.close();
    }

    virtual ~Instance() { }

    ItemTypeId item_type_number() const { return item_types_.size(); }
    Weight capacity() const { return capacity_; }
    Weight weight(ItemTypeId j) const { return item_types_[j].weight; }
    Demand demand(ItemTypeId j) const { return item_types_[j].demand; }
    Demand maximum_demand() const { return demand_max_; }
    Demand total_demand() const { return demand_sum_; }

private:

    void read_bpplib_bpp(std::ifstream& file)
    {
        ItemTypeId n;
        Weight c;
        file >> n >> c;
        set_capacity(c);
        Weight w;
        for (ItemTypeId j = 0; j < n; ++j) {
            file >> w;
            add_item_type(w);
        }
    }

    void read_bpplib_csp(std::ifstream& file)
    {
        ItemTypeId n;
        Weight c;
        file >> n >> c;
        set_capacity(c);
        Weight w;
        Demand q;
        for (ItemTypeId j = 0; j < n; ++j) {
            file >> w >> q;
            add_item_type(w, q);
        }
    }

    std::vector<Item> item_types_;
    Weight capacity_;
    Demand demand_max_ = 0;
    Demand demand_sum_ = 0;

};

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        filled_demands_(instance.item_type_number())
    { }

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<Demand> filled_demands_;

    std::vector<ItemTypeId> kp2csp_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    ItemTypeId n = instance.item_type_number();
    columngenerationsolver::Parameters p(n);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Min;
    p.column_lower_bound = 0;
    p.column_upper_bound = instance.maximum_demand();
    // Row bounds.
    for (ItemTypeId j = 0; j < n; ++j) {
        p.row_lower_bounds[j] = instance.demand(j);
        p.row_upper_bounds[j] = instance.demand(j);
        p.row_coefficient_lower_bounds[j] = 0;
        p.row_coefficient_upper_bounds[j] = instance.demand(j);
    }
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = 2 * instance.maximum_demand();
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));
    return p;
}

std::vector<ColIdx> PricingSolver::initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns)
{
    std::fill(filled_demands_.begin(), filled_demands_.end(), 0);
    for (auto p: fixed_columns) {
        const Column& column = columns[p.first];
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
            RowIdx row_index = column.row_indices[row_pos];
            Value row_coefficient = column.row_coefficients[row_pos];
            filled_demands_[row_index] += value * row_coefficient;
        }
    }
    return {};
}

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    ItemTypeId n = instance_.item_type_number();
    knapsacksolver::Profit mult = 10000;

    // Build subproblem instance.
    knapsacksolver::Instance instance_kp;
    instance_kp.set_capacity(instance_.capacity());
    kp2csp_.clear();
    for (ItemTypeId j = 0; j < n; ++j) {
        knapsacksolver::Profit profit = std::floor(mult * duals[j]);
        if (profit <= 0)
            continue;
        for (Demand q = filled_demands_[j]; q < instance_.demand(j); ++q) {
            instance_kp.add_item(instance_.weight(j), profit);
            kp2csp_.push_back(j);
        }
    }

    // Solve subproblem instance.
    auto output_kp = knapsacksolver::minknap(instance_kp);

    // Retrieve column.
    Column column;
    column.objective_coefficient = 1;
    std::vector<Demand> demands(instance_.item_type_number(), 0);
    for (knapsacksolver::ItemIdx j = 0; j < instance_kp.item_number(); ++j)
        if (output_kp.solution.contains_idx(j))
            demands[kp2csp_[j]]++;
    for (ItemTypeId j = 0; j < n; ++j) {
        if (demands[j] > 0) {
            column.row_indices.push_back(j);
            column.row_coefficients.push_back(demands[j]);
        }
    }
    return {column};
}

}

}
