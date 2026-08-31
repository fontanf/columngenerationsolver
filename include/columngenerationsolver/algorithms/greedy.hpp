#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct GreedyParameters: Parameters
{
    /** Parameters for the column generation sub-problem. */
    ColumnGenerationParameters column_generation_parameters;


    virtual int format_width() const override { return 41; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        //int width = format_width();
        //os
        //    ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({});
        return json;
    }
};

struct GreedyOutput: Output
{
    /** Constructor. */
    GreedyOutput(const Model& model):
        Output(model),
        root_relaxation_solution(SolutionBuilder().set_model(model).build()) { }


    Counter number_of_nodes = 0;

    /** The root node's relaxation solution. */
    Solution root_relaxation_solution;

    /** Cuts still active at the end of the root node's relaxation. */
    std::vector<std::shared_ptr<const Cut>> root_cuts;


    virtual int format_width() const override { return 31; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Number of nodes: " << number_of_nodes << std::endl
            << std::setw(width) << std::left << "Number of root cuts: " << root_cuts.size() << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                {"NumberOfNodes", number_of_nodes},
                {"NumberOfRootCuts", root_cuts.size()},
                });
        return json;
    }
};

const GreedyOutput greedy(
        const Model& model,
        const GreedyParameters& parameters = {});

}
