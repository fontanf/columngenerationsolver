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

    /** Column branched on. */
    std::shared_ptr<const Column> column = nullptr;

    /** Value of the column branched at this node. */
    Value value = 0;

    /** Discrepancy of the node. */
    Value discrepancy = 0;

    /** Sum of the value of each fixed column. */
    Value value_sum = 1;

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

    // Nodes
    auto comp = [](
            const std::shared_ptr<LimitedDiscrepancySearchNode>& node_1,
            const std::shared_ptr<LimitedDiscrepancySearchNode>& node_2)
    {
        if (node_1->discrepancy != node_2->discrepancy)
            return node_1->discrepancy < node_2->discrepancy;
        return node_1->value_sum > node_2->value_sum;
    };
    std::multiset<std::shared_ptr<LimitedDiscrepancySearchNode>, decltype(comp)> nodes(comp);

    // Root node.
    auto root = std::make_shared<LimitedDiscrepancySearchNode>();
    nodes.insert(root);
    bool heuristictreesearch_stop = parameters.heuristictreesearch_stop;

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
        if (heuristictreesearch_stop
                && output.number_of_nodes > 2
                && output.number_of_nodes > 2 * output.maximum_depth)
            break;

        // Update output statistics.
        output.number_of_nodes++;
        output.maximum_discrepancy = (std::max)(
                output.maximum_discrepancy,
                node->discrepancy);

        std::vector<std::pair<std::shared_ptr<const Column>, Value>> fixed_columns;
        auto node_tmp = node;
        std::unordered_set<std::shared_ptr<const Column>> tabu;
        while (node_tmp->parent != NULL) {
            if (node_tmp->value != 0)
                fixed_columns.push_back({node_tmp->column, node_tmp->value});
            tabu.insert(node_tmp->column);
            node_tmp = node_tmp->parent;
        }
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
        if (node->depth == 0) {
            algorithm_formatter.print_column_generation_header();
            column_generation_parameters.iteration_callback = [&algorithm_formatter](
                    const ColumnGenerationOutput& cg_output)
            {
                algorithm_formatter.print_column_generation_iteration(
                        cg_output.number_of_column_generation_iterations,
                        cg_output.relaxation_solution.objective_value(),
                        cg_output.columns.size());
            };
        }
        column_generation_parameters.initial_columns.insert(
                column_generation_parameters.initial_columns.end(),
                output.columns.begin(),
                output.columns.end());
        column_generation_parameters.fixed_columns = &fixed_columns;
        column_generation_parameters.timer = parameters.timer;
        column_generation_parameters.verbosity_level = 0;

        // Solve.
        auto cg_output = column_generation(
                model,
                column_generation_parameters);

        // Update output statistics.
        output.time_lpsolve += cg_output.time_lpsolve;
        output.time_pricing += cg_output.time_pricing;
        output.number_of_column_generation_iterations += cg_output.number_of_column_generation_iterations;
        output.columns.insert(
                output.columns.end(),
                cg_output.columns.begin(),
                cg_output.columns.end());

        //std::cout << "bound " << cg_output.solution_value << std::endl;
        if (parameters.timer.needs_to_end())
            break;

        if (node->depth == 0) {
            algorithm_formatter.print_header();
            Counter cg_it_limit = parameters.column_generation_parameters.maximum_number_of_iterations;
            if (cg_it_limit == -1
                    || cg_output.number_of_column_generation_iterations < cg_it_limit) {
                heuristictreesearch_stop = false;
                algorithm_formatter.update_bound(cg_output.relaxation_solution.objective_value());
            }
        }
        if (cg_output.relaxation_solution.columns().empty()) {
            //std::cout << "no solution" << std::endl;
            continue;
        }

        //std::cout << "x";
        //for (auto p: cg_output.solution)
        //    std::cout << " " << p.first << " " << p.second << ";";
        //std::cout << std::endl;

        // Check bound
        if (output.solution.feasible()) {
            // TODO add a parameter to not cut.
            if (model.objective_sense == optimizationtools::ObjectiveDirection::Minimize
                    && output.solution.objective_value() <= cg_output.solution.objective_value() + FFOT_TOL)
                continue;
            if (model.objective_sense == optimizationtools::ObjectiveDirection::Maximize
                    && output.solution.objective_value() >= cg_output.solution.objective_value() - FFOT_TOL)
                continue;
        }

        //std::cout << "fc";
        //for (auto p: fixed_columns)
        //    std::cout << " " << *(p.first) << " " << p.second << ";";
        //std::cout << std::endl;

        // Compute next column to branch on.
        std::shared_ptr<const Column> column_best = nullptr;
        Value value_best = -1;
        Value diff_best = -1;
        for (auto p: cg_output.relaxation_solution.columns()) {
            const std::shared_ptr<const Column>& column = p.first;
            if (tabu.find(column) != tabu.end())
                continue;
            Value value = p.second;
            if (std::abs(ceil(value)) > FFOT_TOL) {
                if (column_best == nullptr
                        || column_best->branching_priority < column->branching_priority
                        || (column_best->branching_priority == column->branching_priority
                            && diff_best > ceil(value) - value)) {
                    column_best = column;
                    value_best = ceil(value);
                    diff_best = ceil(value) - value;
                }
            }
            if (std::abs(floor(value)) > FFOT_TOL) {
                if (column_best == nullptr
                        || column_best->branching_priority < column->branching_priority
                        || (column_best->branching_priority == column->branching_priority
                            && diff_best > value - floor(value))) {
                    column_best = column;
                    value_best = floor(value);
                    diff_best = value - floor(value);
                }
            }
        }
        //std::cout << "col_best " << col_best
        //    << " value_best " << value_best
        //    << " diff_best " << diff_best
        //    << std::endl;
        if (column_best == nullptr)
            continue;

        // Create children.
        for (Value value = model.column_upper_bound;
                value >= model.column_lower_bound;
                --value) {

            auto child = std::make_shared<LimitedDiscrepancySearchNode>();
            child->parent = node;
            child->column = column_best;
            child->value = value;
            child->value_sum = node->value_sum + value;
            child->discrepancy = node->discrepancy + std::abs(value_best - value);
            child->depth = node->depth + 1;
            nodes.insert(child);

            // Update best solution.
            SolutionBuilder solution_builder;
            solution_builder.set_model(model);
            for (const auto& p: fixed_columns)
                solution_builder.add_column(p.first, p.second);
            solution_builder.add_column(column_best, value);
            Solution solution = solution_builder.build();
            algorithm_formatter.update_solution(solution);
        }

        std::stringstream ss;
        ss << "node " << output.number_of_nodes
            << " depth " << node->depth
            << " disc " << node->discrepancy;
        algorithm_formatter.print(ss.str());

    }

    algorithm_formatter.end();
    return output;
}
