#pragma once

#include "columngenerationsolver/algorithms/column_generation.hpp"

namespace columngenerationsolver
{

struct GreedyParameters: Parameters
{
    /** Parameters for the column generation sub-problem. */
    ColumnGenerationParameters column_generation_parameters;
};

struct GreedyOutput: Output
{
    /** Constructor. */
    GreedyOutput(const Model& model):
        Output(model) { }


    Counter number_of_nodes = 0;


    virtual int format_width() const override { return 31; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Number of nodes: " << number_of_nodes << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                {"NumberOfNodes", number_of_nodes},
                });
        return json;
    }
};

const GreedyOutput greedy(
        const Model& model,
        const GreedyParameters& parameters = {});

}
