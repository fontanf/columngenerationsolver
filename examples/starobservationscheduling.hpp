#pragma once

#include "columngenerationsolver/columngenerationsolver.hpp"

#include "examples/pricingsolver/singlenightstarobservationscheduling.hpp"

/**
 * Star Observation Scheduling Problem.
 *
 * Input:
 * - m nights
 * - n targets with profit wⱼ
 * - a list of possible observations. An observation is associated to a night i
 *   and a target j and has a time-window [rᵢⱼ, dᵢⱼ] and a duration pᵢⱼ such
 *   that 2 pⱼᵢ ≥ dⱼᵢ - rⱼᵢ
 * Problem:
 * - select a list of observations and their starting dates sᵢⱼ such that:
 *   - a target is observed at most once
 *   - observations do not overlap
 *   - starting dates satisfy the time-windows, i.e. rᵢⱼ <= sᵢⱼ and
 *     sᵢⱼ + pᵢⱼ <= dᵢⱼ
 * Objective:
 * - maximize the overall profit of the selected observations
 *
 * The linear programming formulation of the problem based on Dantzig–Wolfe
 * decomposition is written as follows:
 *
 * Variables:
 * - yᵢᵏ ∈ {0, 1} representing a feasible schedule of night i.
 *   yᵢᵏ = 1 iff the corresponding schedule is selected for night i.
 *   xⱼᵢᵏ = 1 iff yᵢᵏ contains target j, otherwise 0.
 *
 * Program:
 *
 * max ∑ᵢ ∑ₖ (∑ⱼ wⱼ xⱼᵢᵏ) yᵢᵏ
 *                                      Note that (∑ⱼ wⱼ xⱼᵢᵏ) is a constant.
 *
 * 0 <= ∑ₖ yᵢᵏ <= 1        for all nights i
 *                         (not more than 1 schedule selected for each night)
 *                                                         Dual variables: uᵢ
 * 0 <= ∑ₖ xⱼᵢᵏ yᵢᵏ <= 1   for all targets j
 *                                        (each target selected at most once)
 *                                                         Dual variables: vⱼ
 *
 * The pricing problem consists in finding a variable of positive reduced cost.
 * The reduced cost of a variable yᵢᵏ is given by:
 * rc(yᵢᵏ) = ∑ⱼ wⱼ xⱼᵢᵏ - uᵢ - ∑ⱼ xⱼᵢᵏ vⱼ
 *         = ∑ⱼ (wⱼ - vⱼ) xⱼᵢᵏ - uᵢ
 *
 * Therefore, finding a variable of maximum reduced cost reduces to solving m
 * Single Night Star Observation Scheduling Problems with targets with profit
 * (wⱼ - vⱼ).
 *
 */

namespace columngenerationsolver
{

namespace starobservationschedulingsolver
{

typedef int64_t TargetId;
typedef int64_t NightId;
typedef int64_t Profit;
typedef int64_t Time;

struct Observable
{
    NightId i;
    TargetId j;
    Time r;
    Time d;
    Time p; // Observation time.
    Profit profit;
};

class Instance
{

public:

    Instance(NightId m, TargetId n): observables_(m), profits_(n) {  }
    void add_observable(const Observable& o) { observables_[o.i].push_back(o); }
    void set_profit(TargetId j, Profit w) { profits_[j] = w; profit_sum_ += w; }

    Instance(std::string instance_path, std::string format = "")
    {
        std::ifstream file(instance_path);
        if (!file.good()) {
            std::cerr << "\033[31m" << "ERROR, unable to open file \"" << instance_path << "\"" << "\033[0m" << std::endl;
            assert(false);
            return;
        }
        if (format == "" || format == "catusse2016") {
            read_catusse2016(file);
        } else {
            std::cerr << "\033[31m" << "ERROR, unknown instance format \"" << format << "\"" << "\033[0m" << std::endl;
        }
        file.close();
    }

    virtual ~Instance() { }

    NightId night_number() const { return observables_.size(); }
    TargetId target_number() const { return profits_.size(); }
    TargetId observable_number(NightId i) const { return observables_[i].size(); }
    const Observable& observable(NightId i, TargetId j_pos) const { return observables_[i][j_pos]; }
    Profit profit(TargetId j) const { return profits_[j]; }
    Profit total_profit() const { return profit_sum_; }

private:

    void read_catusse2016(std::ifstream& file)
    {
        TargetId n;
        NightId m;
        std::string null;
        std::string line;

        std::getline(file, line);
        std::istringstream iss_m(line);
        iss_m >> null >> null >> m;
        observables_ = std::vector<std::vector<Observable>>(m);

        std::getline(file, line);
        std::istringstream iss_n(line);
        iss_n >> null >> null >> n;
        profits_ = std::vector<Profit>(n);

        Profit w;
        for (TargetId j = 0; j < n; ++j) {
            std::getline(file, line);
            std::istringstream iss(line);
            iss >> null >> null >> null >> null >> w;
            set_profit(j, w);

            for (NightId i = 0; i < m; ++i) {
                std::getline(file, line);
                std::istringstream iss(line);
                iss >> null >> null >> null;
                if (iss.eof())
                    continue;
                Observable o;
                o.j = j;
                o.i = i;
                iss >> o.p >> null >> o.r >> null >> null >> null >> o.d;
                add_observable(o);
            }
        }
    }

    std::vector<std::vector<Observable>> observables_;
    std::vector<Profit> profits_;
    Profit profit_sum_ = 0;

};

class PricingSolver: public columngenerationsolver::PricingSolver
{

public:

    PricingSolver(const Instance& instance):
        instance_(instance),
        fixed_targets_(instance.target_number()),
        fixed_nights_(instance.night_number())
    {  }

    virtual std::vector<ColIdx> initialize_pricing(
            const std::vector<Column>& columns,
            const std::vector<std::pair<ColIdx, Value>>& fixed_columns);

    virtual std::vector<Column> solve_pricing(
            const std::vector<Value>& duals);

private:

    const Instance& instance_;

    std::vector<int8_t> fixed_targets_;
    std::vector<int8_t> fixed_nights_;

    std::vector<TargetId> snsosp2sosp_;

};

columngenerationsolver::Parameters get_parameters(
        const Instance& instance,
        columngenerationsolver::LinearProgrammingSolver linear_programming_solver)
{
    NightId m = instance.night_number();
    TargetId n = instance.target_number();
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
    std::fill(fixed_targets_.begin(), fixed_targets_.end(), -1);
    std::fill(fixed_nights_.begin(), fixed_nights_.end(), -1);
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
            if (row_index < instance_.night_number()) {
                fixed_nights_[row_index] = 1;
            } else {
                fixed_targets_[row_index - instance_.night_number()] = 1;
            }
        }
    }
    return {};
}

std::vector<Column> PricingSolver::solve_pricing(
            const std::vector<Value>& duals)
{
    NightId m = instance_.night_number();
    std::vector<Column> columns;
    singlenightstarobservationschedulingsolver::Profit mult = 1000000;
    for (NightId i = 0; i < m; ++i) {
        if (fixed_nights_[i] == 1)
            continue;
        // Build subproblem instance.
        singlenightstarobservationschedulingsolver::Instance instance_snsosp;
        snsosp2sosp_.clear();
        for (TargetId j_pos = 0; j_pos < instance_.observable_number(i); ++j_pos) {
            auto o = instance_.observable(i, j_pos);
            if (fixed_targets_[o.j] == 1)
                continue;
            singlenightstarobservationschedulingsolver::Target target;
            target.profit
                = std::floor(mult * instance_.profit(o.j))
                - std::ceil(mult * duals[m + o.j]);
            if (target.profit <= 0)
                continue;
            target.r = o.r;
            target.d = o.d;
            target.p = o.p;
            instance_snsosp.add_target(target);
            snsosp2sosp_.push_back(o.j);
        }

        // Solve subproblem instance.
        auto output_snsosp = singlenightstarobservationschedulingsolver::dynamicprogramming(instance_snsosp);

        // Retrieve column.
        Column column;
        column.row_indices.push_back(i);
        column.row_coefficients.push_back(1);
        for (const auto& o: output_snsosp.observations()) {
            column.row_indices.push_back(m + snsosp2sosp_[o.j]);
            column.row_coefficients.push_back(1);
            column.objective_coefficient += instance_.profit(snsosp2sosp_[o.j]);
        }
        columns.push_back(column);
    }
    return columns;
}

}

}
