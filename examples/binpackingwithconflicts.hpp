/**
 * Bin Packing Problem with Conflicts.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/binpackingwithconflicts.hpp
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
 * Therefore, finding a variable of minimum reduced cost reduces to solving
 * a Knapsack Problem with Conflicts with items with profit vⱼ.
 *
 */

#pragma once

#include "columngenerationsolver/commons.hpp"

#include "orproblems/binpackingwithconflicts.hpp"
#include "external/treesearchsolver/examples/knapsackwithconflicts.hpp"
#include "external/treesearchsolver/treesearchsolver/iterative_beam_search.hpp"

namespace columngenerationsolver
{

namespace binpackingwithconflicts
{

using namespace orproblems::binpackingwithconflicts;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(
            const Instance& instance,
            double dummy_column_objective_coefficient):
        instance_(instance),
        packed_items_(instance.number_of_items()),
        bpp2kp_(instance.number_of_items()),
        dummy_column_objective_coefficient_(dummy_column_objective_coefficient)
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

    treesearchsolver::NodeId bs_size_of_the_queue_ = 64;

    double dummy_column_objective_coefficient_;

};

columngenerationsolver::Parameters get_parameters(const Instance& instance)
{
    columngenerationsolver::Parameters p(instance.number_of_items());

    p.objective_sense = columngenerationsolver::ObjectiveSense::Min;
    p.column_lower_bound = 0;
    p.column_upper_bound = 1;
    // Row bounds.
    for (ItemId item_id = 0; item_id < instance.number_of_items(); ++item_id) {
        p.row_lower_bounds[item_id] = 1;
        p.row_upper_bounds[item_id] = 1;
        p.row_coefficient_lower_bounds[item_id] = 0;
        p.row_coefficient_upper_bounds[item_id] = 1;
    }
    // Dummy column objective coefficient.
    p.dummy_column_objective_coefficient = 2 * instance.number_of_items();
    // Pricing solver.
    p.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance, p.dummy_column_objective_coefficient));
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
    // Build subproblem instance.
    treesearchsolver::knapsackwithconflicts::Instance instance_kp;
    instance_kp.set_capacity(instance_.capacity());
    kp2bpp_.clear();
    std::fill(bpp2kp_.begin(), bpp2kp_.end(), -1);
    for (ItemId item_id = 0; item_id < instance_.number_of_items(); ++item_id) {
        treesearchsolver::knapsackwithconflicts::Profit profit = duals[item_id];
        if (profit <= 0)
            continue;
        if (packed_items_[item_id] == 1)
            continue;
        bpp2kp_[item_id] = kp2bpp_.size();
        kp2bpp_.push_back(item_id);
        instance_kp.add_item(instance_.item(item_id).weight, duals[item_id]);
        for (ItemId item_id_2: instance_.item(item_id).neighbors)
            if (item_id_2 < item_id && bpp2kp_[item_id_2] != -1)
                instance_kp.add_conflict(bpp2kp_[item_id], bpp2kp_[item_id_2]);
    }
    ItemId kp_n = kp2bpp_.size();

    std::vector<Column> columns;

    // Solve subproblem instance.
    treesearchsolver::knapsackwithconflicts::BranchingScheme branching_scheme(instance_kp, {});
    for (;;) {
        bool ok = false;

        treesearchsolver::IterativeBeamSearchOptionalParameters<treesearchsolver::knapsackwithconflicts::BranchingScheme> kp_parameters;
        kp_parameters.maximum_size_of_the_solution_pool = 100;
        kp_parameters.minimum_size_of_the_queue = bs_size_of_the_queue_;
        kp_parameters.maximum_size_of_the_queue = bs_size_of_the_queue_;
        //parameters_espp.info.set_verbose(true);
        auto kp_output = treesearchsolver::iterative_beam_search(
                branching_scheme, kp_parameters);

        // Retrieve column.
        ItemId i = 0;
        for (const std::shared_ptr<treesearchsolver::knapsackwithconflicts::BranchingScheme::Node>& node:
                kp_output.solution_pool.solutions()) {
            if (i > 2 * kp_n)
                break;
            Column column;
            column.objective_coefficient = 1;
            for (auto node_tmp = node; node_tmp->father != nullptr; node_tmp = node_tmp->father) {
                i++;
                column.row_indices.push_back(kp2bpp_[node_tmp->j]);
                column.row_coefficients.push_back(1);
            }
            columns.push_back(column);

            if (columngenerationsolver::compute_reduced_cost(column, duals)
                    <= -dummy_column_objective_coefficient_ * 10e-9) {
                ok = true;
            }
        }
        if (ok) {
            break;
        } else {
            if (bs_size_of_the_queue_ < 1024) {
                bs_size_of_the_queue_ *= 2;
            } else {
                break;
            }
        }
    }
    return columns;
}

}

}

