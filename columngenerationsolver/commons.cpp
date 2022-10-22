#include "columngenerationsolver/commons.hpp"

using namespace columngenerationsolver;

std::vector<std::pair<Column, Value> >const columngenerationsolver::to_solution(
        const Parameters& parameters,
        const std::vector<std::pair<ColIdx, Value>>& columns)
{
    std::vector<std::pair<Column, Value>> solution;
    for (const auto& p: columns) {
        const Column column = parameters.columns[p.first];
        Value value = p.second;
        solution.push_back({column, value});
    }
    return solution;
}
