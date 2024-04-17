/**
 * Bin packing problem with conflicts.
 *
 * Problem description:
 * See https://github.com/fontanf/orproblems/blob/main/orproblems/bin_packing_with_conflicts.hpp
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

#include "orproblems/packing/bin_packing_with_conflicts.hpp"
#include "treesearchsolver/examples/knapsack_with_conflicts.hpp"
#include "treesearchsolver/iterative_beam_search.hpp"

namespace columngenerationsolver
{

namespace bin_packing_with_conflicts
{

using namespace orproblems::bin_packing_with_conflicts;

using Node = treesearchsolver::knapsack_with_conflicts::BranchingScheme::Node;

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(
            const Instance& instance):
        instance_(instance),
        packed_items_(instance.number_of_items()),
        bpp2kp_(instance.number_of_items())
    { }

    virtual inline std::vector<std::shared_ptr<const Column>> initialize_pricing(
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns);

    virtual inline std::vector<std::shared_ptr<const Column>> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<int8_t> packed_items_;

    std::vector<ItemId> kp2bpp_;

    std::vector<treesearchsolver::knapsack_with_conflicts::ItemId> bpp2kp_;

    treesearchsolver::NodeId bs_size_of_the_queue_ = 1024;

};

inline columngenerationsolver::Model get_model(const Instance& instance)
{
    columngenerationsolver::Model model;

    model.objective_sense = optimizationtools::ObjectiveDirection::Minimize;

    // Row bounds.
    for (ItemId item_id = 0; item_id < instance.number_of_items(); ++item_id) {
        Row row;
        row.lower_bound = 1;
        row.upper_bound = 1;
        row.coefficient_lower_bound = 0;
        row.coefficient_upper_bound = 1;
        model.rows.push_back(row);
    }

    // Pricing solver.
    model.pricing_solver = std::unique_ptr<columngenerationsolver::PricingSolver>(
            new PricingSolver(instance));

    return model;
}

std::vector<std::shared_ptr<const Column>> PricingSolver::initialize_pricing(
            const std::vector<std::pair<std::shared_ptr<const Column>, Value>>& fixed_columns)
{
    std::fill(packed_items_.begin(), packed_items_.end(), 0);
    for (auto p: fixed_columns) {
        const Column& column = *(p.first);
        Value value = p.second;
        if (value < 0.5)
            continue;
        for (const LinearTerm& element: column.elements)
            packed_items_[element.row] += value * element.coefficient;
    }
    return {};
}

std::vector<std::shared_ptr<const Column>> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    // Build subproblem instance.
    treesearchsolver::knapsack_with_conflicts::InstanceBuilder kp_instance_builder;
    kp_instance_builder.set_capacity(instance_.capacity());
    kp2bpp_.clear();
    std::fill(bpp2kp_.begin(), bpp2kp_.end(), -1);
    for (ItemId item_id = 0; item_id < instance_.number_of_items(); ++item_id) {
        treesearchsolver::knapsack_with_conflicts::Profit profit = duals[item_id];
        if (profit <= 0)
            continue;
        if (packed_items_[item_id] == 1)
            continue;
        bpp2kp_[item_id] = kp2bpp_.size();
        kp2bpp_.push_back(item_id);
        kp_instance_builder.add_item(instance_.item(item_id).weight, duals[item_id]);
        for (ItemId item_id_2: instance_.item(item_id).neighbors)
            if (item_id_2 < item_id && bpp2kp_[item_id_2] != -1)
                kp_instance_builder.add_conflict(bpp2kp_[item_id], bpp2kp_[item_id_2]);
    }
    const orproblems::knapsack_with_conflicts::Instance kp_instance = kp_instance_builder.build();

    std::vector<std::shared_ptr<const Column>> columns;

    // Solve subproblem instance.
    treesearchsolver::knapsack_with_conflicts::BranchingScheme branching_scheme(kp_instance, {});

    treesearchsolver::IterativeBeamSearchParameters<treesearchsolver::knapsack_with_conflicts::BranchingScheme> kp_parameters;
    kp_parameters.verbosity_level = 0;
    kp_parameters.maximum_size_of_the_solution_pool = 1;
    kp_parameters.minimum_size_of_the_queue = bs_size_of_the_queue_;
    kp_parameters.maximum_size_of_the_queue = bs_size_of_the_queue_;
    auto kp_output = treesearchsolver::iterative_beam_search(
            branching_scheme, kp_parameters);

    // Retrieve column.
    for (const std::shared_ptr<Node>& node: kp_output.solution_pool.solutions()) {
        Column column;
        column.objective_coefficient = 1;
        for (auto node_tmp = node;
                node_tmp->parent != nullptr;
                node_tmp = node_tmp->parent) {
            LinearTerm element;
            element.row = kp2bpp_[node_tmp->item_id];
            element.coefficient = 1;
            column.elements.push_back(element);
        }
        columns.push_back(std::shared_ptr<const Column>(new Column(column)));
    }
    return columns;
}

inline void write_solution(
        const Solution& solution,
        const std::string& certificate_path)
{
    std::ofstream file(certificate_path);
    if (!file.good()) {
        throw std::runtime_error(
                "Unable to open file \"" + certificate_path + "\".");
    }

    file << solution.columns().size() << std::endl;
    for (auto colval: solution.columns()) {
        const Column& column = *(colval.first);
        file << column.elements.size() << " ";
        for (const LinearTerm& element: column.elements)
            file << " " << element.row;
        file << std::endl;
    }
}

}
}
