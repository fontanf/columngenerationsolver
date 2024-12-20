#include "columngenerationsolver/algorithms/limited_discrepancy_search.hpp"

#include "columngenerationsolver/algorithm_formatter.hpp"

#include <unordered_set>
#include <set>

using namespace columngenerationsolver;

namespace
{

struct LimitedDiscrepancySearchNode
{
    /** Parent node. */
    std::shared_ptr<LimitedDiscrepancySearchNode> parent = nullptr;

    /** Relaxation solution. */
    std::shared_ptr<Solution> relaxation_solution;

    /** Columns fixed at this node. */
    ColumnMap fixed_columns;

    /** Column branched on at this node. */
    std::shared_ptr<const Column> column = nullptr;

    /** Discrepancy of the node. */
    Value discrepancy = 0;

    /** Depth of the node. */
    ColIdx depth = 0;
};

}

const LimitedDiscrepancySearchOutput columngenerationsolver::limited_discrepancy_search(
        const Model& model,
        const LimitedDiscrepancySearchParameters& parameters)
{
    // Initial display.
    LimitedDiscrepancySearchOutput output(model);
    AlgorithmFormatter algorithm_formatter(
            model,
            parameters,
            output);
    algorithm_formatter.start("Limited discrepancy search");
    output.dummy_column_objective_coefficient = parameters.dummy_column_objective_coefficient;

    std::vector<std::shared_ptr<const Column>> column_pool = parameters.column_pool;

    ColumnHasher column_hasher(model);

    // Nodes
    auto comp = [](
            const std::shared_ptr<LimitedDiscrepancySearchNode>& node_1,
            const std::shared_ptr<LimitedDiscrepancySearchNode>& node_2)
    {
        if (node_1->discrepancy != node_2->discrepancy)
            return node_1->discrepancy < node_2->discrepancy;
        return node_1->depth > node_2->depth;
    };
    std::multiset<std::shared_ptr<LimitedDiscrepancySearchNode>, decltype(comp)> nodes(comp);

    // Root node.
    auto root = std::make_shared<LimitedDiscrepancySearchNode>();
    nodes.insert(root);

    while (!nodes.empty()) {
        //std::cout << "nodes.size() " << nodes.size() << std::endl;

        // Check end.
        if (parameters.timer.needs_to_end())
            break;

        if (output.solution.feasible()
                && std::abs(output.solution.objective_value() - output.bound) < FFOT_TOL)
            break;

        // Get node
        auto node = *nodes.begin();
        nodes.erase(nodes.begin());

        // Check discrepancy limit.
        if (!parameters.continue_until_feasible
                || !output.solution.columns().empty())
            if (node->discrepancy > parameters.discrepancy_limit)
                break;
        if (output.maximum_depth < node->depth - node->discrepancy)
            output.maximum_depth = node->depth - node->discrepancy;
        if (parameters.automatic_stop
                && output.number_of_nodes >= 2
                && output.number_of_nodes > 4 * output.maximum_depth)
            break;

        // Update output statistics.
        output.number_of_nodes++;
        output.maximum_discrepancy = (std::max)(
                output.maximum_discrepancy,
                node->discrepancy);

        //std::cout
        //    << "t " << parameters.info.elapsed_time()
        //    << " node " << output.number_of_nodes
        //    << " / " << nodes.size()
        //    << " diff " << node->discrepancy
        //    << " depth " << node->depth
        //    << " col " << node->col
        //    << " value " << node->value
        //    << " value_sum " << node->value_sum
        //    << " fixed_columns.size() " << fixed_columns.size()
        //    << std::endl;
        //if (node->parent != NULL)
        //    std::cout << node->value << " " << model.columns[node->col] << std::endl;

        // Run column generation
        ColumnGenerationParameters column_generation_parameters
            = parameters.column_generation_parameters;
        column_generation_parameters.timer = parameters.timer;
        column_generation_parameters.verbosity_level = 0;
        column_generation_parameters.dummy_column_objective_coefficient
            = output.dummy_column_objective_coefficient;
        if (parameters.internal_diving == 2
                || (parameters.internal_diving == 1 && node->depth == 0)) {
            column_generation_parameters.internal_diving = 1;
        }
        if (node->depth == 0) {
            algorithm_formatter.print_column_generation_header();
            column_generation_parameters.iteration_callback = [&algorithm_formatter](
                    const ColumnGenerationOutput& cg_output)
            {
                algorithm_formatter.print_column_generation_iteration(
                        cg_output.number_of_column_generation_iterations,
                        cg_output.number_of_columns_in_linear_subproblem,
                        cg_output.relaxation_solution_value,
                        cg_output.bound);
            };
            column_generation_parameters.new_bound_callback = [&algorithm_formatter](
                    const Output& cg_output)
            {
                algorithm_formatter.update_bound(cg_output.bound);
            };
        }
        if (node->parent == nullptr) {
            column_generation_parameters.initial_columns = parameters.initial_columns;
        } else {
            for (const auto& p: node->parent->relaxation_solution->columns()) {
                bool ok = true;
                for (const auto& column: model.columns)
                    if (p.first.get() == column.get())
                        ok = false;
                if (!ok)
                    continue;
                column_generation_parameters.initial_columns.push_back(p.first);
            }
        }
        column_generation_parameters.column_pool = column_pool;
        column_generation_parameters.fixed_columns = node->fixed_columns.columns();

        // Solve.
        auto cg_output = column_generation(
                model,
                column_generation_parameters);

        // Update output statistics.
        output.time_lpsolve += cg_output.time_lpsolve;
        output.time_pricing += cg_output.time_pricing;
        output.dummy_column_objective_coefficient = cg_output.dummy_column_objective_coefficient;
        output.number_of_column_generation_iterations += cg_output.number_of_column_generation_iterations;
        output.columns.insert(
                output.columns.end(),
                cg_output.columns.begin(),
                cg_output.columns.end());
        column_pool.insert(
                column_pool.end(),
                cg_output.columns.begin(),
                cg_output.columns.end());

        //std::cout << "bound " << cg_output.solution_value << std::endl;
        if (parameters.timer.needs_to_end())
            break;

        if (node->depth == 0) {
            algorithm_formatter.print_header();
            algorithm_formatter.update_bound(cg_output.bound);
            output.relaxation_solution = cg_output.relaxation_solution;
        }
        if (!cg_output.relaxation_solution.feasible_relaxation()) {
            //std::cout << "no solution" << std::endl;
            continue;
        }

        //std::cout << "x";
        //for (auto p: cg_output.relaxation_solution.columns())
        //    std::cout << " " << p.first << " " << p.second << ";";
        //std::cout << std::endl;

        // Check bound
        if (output.solution.feasible()
                && parameters.bound) {
            if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                    && output.solution.objective_value() <= cg_output.relaxation_solution.objective_value() + FFOT_TOL) {
                continue;
            }
            if (model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                    && output.solution.objective_value() >= cg_output.relaxation_solution.objective_value() - FFOT_TOL) {
                continue;
            }
        }

        // Update node relaxation solution.
        node->relaxation_solution = std::shared_ptr<Solution>(new Solution(cg_output.relaxation_solution));

        // If the relaxation is (integer) feasible, save the solution and stop.
        std::stringstream ss;
        ss << "node " << output.number_of_nodes
            << " depth " << node->depth
            << " disc " << node->discrepancy;
        if (cg_output.relaxation_solution.feasible()) {
            algorithm_formatter.update_solution(cg_output.relaxation_solution);
            algorithm_formatter.print(ss.str());
            continue;
        }

        // Try rounded solution.
        SolutionBuilder rounded_solution_builder;
        rounded_solution_builder.set_model(model);
        for (auto p: cg_output.relaxation_solution.columns()) {
            if (p.first->type == VariableType::Continuous) {
                rounded_solution_builder.add_column(p.first, p.second);
            } else {
                if (std::round(p.second) != 0)
                    rounded_solution_builder.add_column(p.first, std::round(p.second));
            }
        }
        Solution rounded_solution = rounded_solution_builder.build();
        if (rounded_solution.feasible())
            algorithm_formatter.update_solution(rounded_solution);

        algorithm_formatter.print(ss.str());

        //std::cout << "fc";
        //for (auto p: fixed_columns.columns())
        //    std::cout << " " << *(p.first) << " " << p.second << ";";
        //std::cout << std::endl;

        // Fix columns with value >= 1 to their floor value.
        ColumnMap fixed_columns = node->fixed_columns;
        bool fixed_found = false;
        for (auto p: cg_output.relaxation_solution.columns()) {
            const std::shared_ptr<const Column>& column = p.first;
            if (p.first->type == VariableType::Continuous)
                continue;
            Value value = std::floor(p.second);
            if (value <= fixed_columns.get_column_value(column))
                continue;
            fixed_columns.set_column_value(column, value);
            fixed_found = true;
        }

        if (fixed_found) {
            // Create child node and add them to the queue.
            auto child = std::make_shared<LimitedDiscrepancySearchNode>();
            child->parent = node;
            child->fixed_columns = fixed_columns;
            child->column = nullptr;
            child->discrepancy = node->discrepancy;
            child->depth = node->depth + 1;
            nodes.insert(child);
            continue;
        }

        // Get the set of columns on which branching is forbidden.
        auto node_tmp = node;
        std::unordered_set<std::shared_ptr<const Column>> tabu;
        for (auto node_tmp = node;
                node_tmp->parent != NULL;
                node_tmp = node_tmp->parent) {
            if (node_tmp->column != nullptr)
                tabu.insert(node_tmp->column);
        }

        // Compute next column to branch on.
        std::shared_ptr<const Column> column_best = nullptr;
        Value value_best = -1;
        Value diff_best = -1;
        for (auto p: cg_output.relaxation_solution.columns()) {
            const std::shared_ptr<const Column>& column = p.first;
            Value value = p.second;

            // Don't branch on continuous variables.
            if (p.first->type == VariableType::Continuous)
                continue;

            // Don't branch on a fixed column.
            if (value <= fixed_columns.get_column_value(column))
                continue;

            // Don't fix a column to 0.
            if (ceil(value) == 0)
                continue;

            // Don't branch on a column which has already been branched on.
            if (tabu.find(column) != tabu.end())
                continue;

            if (column_best == nullptr
                    || column_best->branching_priority < column->branching_priority
                    || (column_best->branching_priority == column->branching_priority
                        && diff_best > ceil(value) - value)) {
                column_best = column;
                value_best = ceil(value);
                diff_best = ceil(value) - value;
            }
        }
        //std::cout << "col_best " << col_best
        //    << " value_best " << value_best
        //    << " diff_best " << diff_best
        //    << std::endl;
        if (column_best == nullptr)
            continue;

        // Create child nodes and add them to the queue.
        auto child_1 = std::make_shared<LimitedDiscrepancySearchNode>();
        auto child_2 = std::make_shared<LimitedDiscrepancySearchNode>();
        child_1->parent = node;
        child_2->parent = node;
        child_1->fixed_columns = fixed_columns;
        child_1->fixed_columns.set_column_value(column_best, value_best);
        child_2->fixed_columns = fixed_columns;
        child_1->column = column_best;
        child_2->column = column_best;
        child_1->discrepancy = node->discrepancy;
        child_2->discrepancy = node->discrepancy + 1;
        child_1->depth = node->depth + 1;
        child_2->depth = node->depth + 1;
        nodes.insert(child_1);
        nodes.insert(child_2);
    }

    algorithm_formatter.end();
    return output;
}
