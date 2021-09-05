#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct LimitedDiscrepancySearchOutput
{
    std::vector<std::pair<Column, Value>> solution;
    Value solution_value = 0;
    Value solution_discrepancy = -1;
    Value bound;
    Counter number_of_nodes = 0;
    Counter maximum_depth = 0;
    double time_lpsolve = 0.0;
    double time_pricing = 0.0;
    Counter total_number_of_columns = 0;
    Counter number_of_added_columns = 0;
};

typedef std::function<void(const LimitedDiscrepancySearchOutput&)> LimitedDiscrepancySearchCallback;

struct LimitedDiscrepancySearchOptionalParameters
{
    LimitedDiscrepancySearchCallback new_bound_callback
        = [](const LimitedDiscrepancySearchOutput& o) { (void)o; };
    Counter number_of_threads = 3;
    Value discrepancy_limit = std::numeric_limits<Value>::infinity();
    bool heuristictreesearch_stop = false;
    bool* end = NULL;
    ColumnGenerationOptionalParameters columngeneration_parameters;
    optimizationtools::Info info = optimizationtools::Info();
};

struct LimitedDiscrepancySearchNode
{
    std::shared_ptr<LimitedDiscrepancySearchNode> father = nullptr;
    ColIdx col = -1;
    Value value = 0;
    Value discrepancy = 0;
    Value value_sum = 1;
    ColIdx depth = 0;
};

inline LimitedDiscrepancySearchOutput limiteddiscrepancysearch(
        Parameters& parameters,
        LimitedDiscrepancySearchOptionalParameters optional_parameters = {})
{
    VER(optional_parameters.info, "*** limiteddiscrepancysearch ***" << std::endl);
    VER(optional_parameters.info, "---" << std::endl);
    VER(optional_parameters.info, "Linear programming solver:                " << optional_parameters.columngeneration_parameters.linear_programming_solver << std::endl);
    VER(optional_parameters.info, "Discrepancy limit:                        " << optional_parameters.discrepancy_limit << std::endl);
    VER(optional_parameters.info, "Static Wentges smoothing parameter:       " << optional_parameters.columngeneration_parameters.static_wentges_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Static directional smoothing parameter:   " << optional_parameters.columngeneration_parameters.static_directional_smoothing_parameter << std::endl);
    VER(optional_parameters.info, "Self-adjusting Wentges smoothing:         " << optional_parameters.columngeneration_parameters.self_adjusting_wentges_smoothing << std::endl);
    VER(optional_parameters.info, "Automatic directional smoothing:          " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "Column generation iteration limit:        " << optional_parameters.columngeneration_parameters.automatic_directional_smoothing << std::endl);
    VER(optional_parameters.info, "---" << std::endl);

    LimitedDiscrepancySearchOutput output;
    output.solution_value = (parameters.objective_sense == ObjectiveSense::Min)?
        std::numeric_limits<Value>::infinity():
        -std::numeric_limits<Value>::infinity();
    output.bound = (parameters.objective_sense == ObjectiveSense::Min)?
        -std::numeric_limits<Value>::infinity():
        std::numeric_limits<Value>::infinity();
    display_initialize(parameters, optional_parameters.info);

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
    bool heuristictreesearch_stop = optional_parameters.heuristictreesearch_stop;

    while (!nodes.empty()) {
        output.number_of_nodes++;
        //std::cout << "nodes.size() " << nodes.size() << std::endl;

        // Check time.
        if (optional_parameters.info.needs_to_end())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;
        if (std::abs(output.solution_value - output.bound) < TOL)
            break;

        // Get node
        auto node = *nodes.begin();
        nodes.erase(nodes.begin());
        // Check discrepancy limit.
        if (node->discrepancy > optional_parameters.discrepancy_limit)
            break;
        if (output.maximum_depth < node->depth - node->discrepancy)
            output.maximum_depth = node->depth - node->discrepancy;
        if (heuristictreesearch_stop
                && output.number_of_nodes > 2
                && output.number_of_nodes > 2 * output.maximum_depth)
            break;

        std::vector<std::pair<ColIdx, Value>> fixed_columns;
        auto node_tmp = node;
        std::vector<uint8_t> tabu(parameters.columns.size(), 0);
        while (node_tmp->father != NULL) {
            if (node_tmp->value != 0)
                fixed_columns.push_back({node_tmp->col, node_tmp->value});
            tabu[node_tmp->col] = 1;
            node_tmp = node_tmp->father;
        }
        //std::cout
        //    << "t " << optional_parameters.info.elapsed_time()
        //    << " node " << output.number_of_nodes
        //    << " / " << nodes.size()
        //    << " diff " << node->discrepancy
        //    << " depth " << node->depth
        //    << " col " << node->col
        //    << " value " << node->value
        //    << " value_sum " << node->value_sum
        //    << " fixed_columns.size() " << fixed_columns.size()
        //    << std::endl;
        //if (node->father != NULL)
        //    std::cout << node->value << " " << parameters.columns[node->col] << std::endl;

        // Run column generation
        ColumnGenerationOptionalParameters columngeneration_parameters
            = optional_parameters.columngeneration_parameters;
        columngeneration_parameters.fixed_columns = &fixed_columns;
        columngeneration_parameters.end = optional_parameters.end;
        columngeneration_parameters.info.reset_time();
        columngeneration_parameters.info.set_time_limit(optional_parameters.info.remaining_time());
        //columngeneration_parameters.info.set_verbose(true);
        auto output_columngeneration = columngeneration(
                parameters,
                columngeneration_parameters);
        output.time_lpsolve += output_columngeneration.time_lpsolve;
        output.time_pricing += output_columngeneration.time_pricing;
        output.number_of_added_columns += output_columngeneration.number_of_added_columns;
        //std::cout << "bound " << output_columngeneration.solution_value << std::endl;
        if (optional_parameters.info.needs_to_end())
            break;
        if (optional_parameters.end != NULL && *optional_parameters.end == true)
            break;
        if (node->depth == 0) {
            Counter cg_it_limit = optional_parameters.columngeneration_parameters.iteration_limit;
            if (cg_it_limit == -1 || output_columngeneration.number_of_iterations < cg_it_limit) {
                heuristictreesearch_stop = false;
                output.bound = output_columngeneration.solution_value;
                display(output.solution_value, output.bound, std::stringstream("root node"), optional_parameters.info);
                optional_parameters.new_bound_callback(output);
            }
        }
        if (output_columngeneration.solution.size() == 0) {
            continue;
        }

        //std::cout << "x";
        //for (auto p: output_columngeneration.solution)
        //    std::cout << " " << p.first << " " << p.second << ";";
        //std::cout << std::endl;

        // Check bound
        if (output.solution.size() > 0) {
            if (parameters.objective_sense == ObjectiveSense::Min
                    && output.solution_value <= output_columngeneration.solution_value + TOL)
                continue;
            if (parameters.objective_sense == ObjectiveSense::Max
                    && output.solution_value >= output_columngeneration.solution_value - TOL)
                continue;
        }

        //std::cout << "fc";
        //for (auto p: fixed_columns)
        //    std::cout << " " << p.first << " " << p.second << ";";
        //std::cout << std::endl;

        // Compute next column to branch on.
        ColIdx col_best = -1;
        Value val_best = -1;
        Value diff_best = -1;
        Value bp_best = -1;
        for (auto p: output_columngeneration.solution) {
            ColIdx col = p.first;
            if (col < (ColIdx)tabu.size() && tabu[col] == 1)
                continue;
            Value val = p.second;
            Value bp = parameters.columns[col].branching_priority;
            if (ceil(val) != 0) {
                if (col_best == -1
                        || bp_best < bp
                        || (bp_best == bp && diff_best > ceil(val) - val)) {
                    col_best = col;
                    val_best = ceil(val);
                    diff_best = ceil(val) - val;
                    bp_best = bp;
                }
            }
            if (floor(val) != 0) {
                if (col_best == -1
                        || bp_best < bp
                        || (bp_best == bp && diff_best > val - floor(val))) {
                    col_best = col;
                    val_best = floor(val);
                    diff_best = val - floor(val);
                    bp_best = bp;
                }
            }
        }
        //std::cout << "col_best " << col_best
        //    << " val_best " << val_best
        //    << " diff_best " << diff_best
        //    << std::endl;
        if (col_best == -1)
            continue;

        // Create children.
        for (Value value = parameters.column_lower_bound; value <= parameters.column_upper_bound; ++value) {
            auto child = std::make_shared<LimitedDiscrepancySearchNode>();
            child->father      = node;
            child->col         = col_best;
            child->value       = value;
            child->value_sum   = node->value_sum + value;
            child->discrepancy = node->discrepancy + abs(val_best - value);
            child->depth       = node->depth + 1;
            nodes.insert(child);

            // Update best solution.
            fixed_columns.push_back({col_best, value});
            if (is_feasible(parameters, fixed_columns)) {
                if (output.solution.size() == 0) {
                    output.solution = to_solution(parameters, fixed_columns);
                    output.solution_value = compute_value(parameters, fixed_columns);
                    output.solution_discrepancy = node->discrepancy;
                    //std::cout << "New best solution value " << output.solution_value << std::endl;
                    std::stringstream ss;
                    ss << "node " << output.number_of_nodes << " discrepancy " << output.solution_discrepancy;
                    display(output.solution_value, output.bound, ss, optional_parameters.info);
                    optional_parameters.new_bound_callback(output);
                } else {
                    Value solution_value = compute_value(parameters, fixed_columns);
                    if (parameters.objective_sense == ObjectiveSense::Min
                            && output.solution_value - TOL > solution_value) {
                        //std::cout << "New best solution value " << solution_value << std::endl;
                        output.solution = to_solution(parameters, fixed_columns);
                        output.solution_value = solution_value;
                        output.solution_discrepancy = node->discrepancy;
                        std::stringstream ss;
                        ss << "node " << output.number_of_nodes << " discrepancy " << output.solution_discrepancy;
                        display(output.solution_value, output.bound, ss, optional_parameters.info);
                        optional_parameters.new_bound_callback(output);
                    }
                    if (parameters.objective_sense == ObjectiveSense::Max
                            && output.solution_value + TOL < solution_value) {
                        output.solution = to_solution(parameters, fixed_columns);
                        output.solution_value = solution_value;
                        output.solution_discrepancy = node->discrepancy;
                        std::stringstream ss;
                        ss << "node " << output.number_of_nodes << " discrepancy " << output.solution_discrepancy;
                        display(output.solution_value, output.bound, ss, optional_parameters.info);
                        optional_parameters.new_bound_callback(output);
                    }
                }
            }
            fixed_columns.pop_back();
        }

    }

    output.total_number_of_columns = parameters.columns.size();
    display_end(output, optional_parameters.info);
    VER(optional_parameters.info, "Node number:          " << output.number_of_nodes << std::endl);
    return output;
}

}
