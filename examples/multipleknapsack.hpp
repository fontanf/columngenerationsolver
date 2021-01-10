#pragma once

#include "columngenerationsolver/columngenerationsolver.hpp"

#include "knapsacksolver/algorithms/minknap.hpp"

/**
 * Multiple Knapsack Problem.
 *
 * Input:
 * - m containers (knapsacks) with capacity cᵢ (i = 1..m)
 * - n items with profit pⱼ and weight wⱼ (j = 1..n)
 * Problem:
 * - select m disjoint subsets of items (one per knapsack) such that the total
 *   weight of the items in a knapsack does not exceed its capacity.
 * Objective:
 * - Maximize the overall profit of the selected items.
 *
 * The linear programming formulation of the problem based on Dantzig–Wolfe
 * decomposition is written as follows:
 *
 * Variables:
 * - yᵢᵏ ∈ {0, 1} representing a set of items for knapsack i.
 *   yᵢᵏ = 1 iff the corresponding set of items is selected for knapsack i.
 *   xⱼᵢᵏ = 1 iff yᵢᵏ contains item j, otherwise 0.
 *
 * Program:
 *
 * max ∑ᵢ ∑ₖ (∑ⱼ cⱼ xⱼᵢᵏ) yᵢᵏ
 *                                      Note that (∑ⱼ cⱼ xⱼᵢᵏ) is a constant.
 *
 * 0 <= ∑ₖ yᵢᵏ <= 1        for all knapsack i
 *                       (not more than 1 packing selected for each knapsack)
 *                                                         Dual variables: uᵢ
 * 0 <= ∑ₖ xⱼᵢᵏ yᵢᵏ <= 1   for all items j
 *                                          (each item selected at most once)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of positive reduced cost.
 * The reduced cost of a variable yᵢᵏ is given by:
 * rc(yᵢᵏ) = ∑ⱼ cⱼ xⱼᵢᵏ - uᵢ - ∑ⱼ xⱼᵢᵏ vⱼ
 *         = ∑ⱼ (cⱼ - vⱼ) xⱼᵢᵏ - uᵢ
 *
 * Therefore, finding a variable of maximum reduced cost reduces to solving
 * m Knapsack Problems with items with profit (cⱼ - vⱼ).
 *
 */

namespace columngenerationsolver
{

namespace multipleknapsacksolver
{

typedef int64_t ItemId;
typedef int64_t KnapsackId;
typedef int64_t Weight;
typedef int64_t Profit;

struct Item
{
    Weight weight;
    Profit profit;
};

class Instance
{

public:

    Instance() {  }
    void add_knapsack(Weight capacity) { capacities_.push_back(capacity); }
    void add_item(Weight weight, Profit profit)
    {
        items_.push_back(Item{weight, profit});
        profit_sum_ += profit;
    }

    Instance(std::string instance_path, std::string format = "")
    {
        std::ifstream file(instance_path);
        if (!file.good()) {
            std::cerr << "\033[31m" << "ERROR, unable to open file \"" << instance_path << "\"" << "\033[0m" << std::endl;
            assert(false);
            return;
        }
        if (format == "" || format == "dellamico2018") {
            read_dellamico2018(file);
        } else {
            std::cerr << "\033[31m" << "ERROR, unknown instance format \"" << format << "\"" << "\033[0m" << std::endl;
        }
        file.close();
    }

    virtual ~Instance() { }

    KnapsackId knapsack_number() const { return capacities_.size(); }
    ItemId item_number() const { return items_.size(); }
    Weight weight(ItemId j) const { return items_[j].weight; }
    Weight profit(ItemId j) const { return items_[j].profit; }
    Weight capacity(KnapsackId i) const { return capacities_[i]; }
    Profit total_profit() const { return profit_sum_; }

private:

    void read_dellamico2018(std::ifstream& file)
    {
        KnapsackId m;
        ItemId n;
        file >> m >> n;
        Weight c;
        for (KnapsackId i = 0; i < m; ++i) {
            file >> c;
            add_knapsack(c);
        }
        Weight w;
        Profit p;
        for (ItemId j = 0; j < n; ++j) {
            file >> w >> p;
            add_item(w, p);
        }
    }

    std::vector<Item> items_;
    std::vector<Weight> capacities_;
    Profit profit_sum_ = 0;

};

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        fixed_items_(instance.item_number()),
        fixed_knapsacks_(instance.knapsack_number())
    {  }

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<int8_t> fixed_items_;
    std::vector<int8_t> fixed_knapsacks_;

    std::vector<ItemId> kp2mkp_;

};

columngenerationsolver::Parameters get_parameters(
        const Instance& instance,
        columngenerationsolver::LinearProgrammingSolver linear_programming_solver)
{
    KnapsackId m = instance.knapsack_number();
    ItemId n = instance.item_number();
    columngenerationsolver::Parameters p(m + n);

    p.objective_sense = columngenerationsolver::ObjectiveSense::Max;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row lower bounds.
    std::fill(p.row_lower_bounds.begin(), p.row_lower_bounds.begin() + m, 0);
    std::fill(p.row_lower_bounds.begin() + m, p.row_lower_bounds.end(), 0);
    // Row upper bounds.
    std::fill(p.row_upper_bounds.begin(), p.row_upper_bounds.begin() + m, 1);
    std::fill(p.row_upper_bounds.begin() + m, p.row_upper_bounds.end(), 1);
    // Row coefficent lower bounds.
    std::fill(p.row_coefficient_lower_bounds.begin(), p.row_coefficient_lower_bounds.end(), 0);
    // Row coefficent upper bounds.
    std::fill(p.row_coefficient_upper_bounds.begin(), p.row_coefficient_upper_bounds.end(), 1);
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = -instance.total_profit();
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));
    p.linear_programming_solver = linear_programming_solver;
    return p;
}

std::vector<ColIdx> PricingSolver::initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns)
{
    std::fill(fixed_items_.begin(), fixed_items_.end(), -1);
    std::fill(fixed_knapsacks_.begin(), fixed_knapsacks_.end(), -1);
    for (auto p: fixed_columns) {
        const Column& column = columns[p.first];
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (RowIdx row_pos = 0; row_pos < (RowIdx)column.row_indices.size(); ++row_pos) {
            RowIdx row_index = column.row_indices[row_pos];
            Value row_coefficient = column.row_coefficients[row_pos];
            if (row_coefficient < 0.5)
                continue;
            if (row_index < instance_.knapsack_number()) {
                fixed_knapsacks_[row_index] = 1;
            } else {
                fixed_items_[row_index - instance_.knapsack_number()] = 1;
            }
        }
    }
    return {};
}

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    KnapsackId m = instance_.knapsack_number();
    ItemId n = instance_.item_number();
    std::vector<Column> columns;
    knapsacksolver::Profit mult = 10000;
    for (KnapsackId i = 0; i < m; ++i) {
        if (fixed_knapsacks_[i] == 1)
            continue;
        // Build knapsack instance.
        knapsacksolver::Instance instance_kp;
        instance_kp.set_capacity(instance_.capacity(i));
        kp2mkp_.clear();
        for (ItemId j = 0; j < n; ++j) {
            if (fixed_items_[j] == 1)
                continue;
            knapsacksolver::Profit profit = std::floor(mult * instance_.profit(j))
                - std::ceil(mult * duals[m + j]);
            if (profit <= 0 || instance_.weight(j) > instance_.capacity(i))
                continue;
            instance_kp.add_item(instance_.weight(j), profit);
            kp2mkp_.push_back(j);
        }

        // Solve knapsack instance.
        auto output_kp = knapsacksolver::minknap(instance_kp);

        // Retrieve column.
        Column column;
        column.row_indices.push_back(i);
        column.row_coefficients.push_back(1);
        for (knapsacksolver::ItemIdx j = 0; j < instance_kp.item_number(); ++j) {
            if (output_kp.solution.contains_idx(j)) {
                column.row_indices.push_back(m + kp2mkp_[j]);
                column.row_coefficients.push_back(1);
                column.objective_coefficient += instance_.profit(kp2mkp_[j]);
            }
        }
        columns.push_back(column);
    }
    return columns;
}

}

}
