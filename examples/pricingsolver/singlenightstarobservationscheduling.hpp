#pragma once

#include "optimizationtools/info.hpp"

/**
 * Single Night Star Observation Scheduling Problem.
 *
 * Input:
 * - n targets with profit wⱼ, time-window [rⱼ, dⱼ] and duration pⱼ such
 *   that 2 pⱼ ≥ dⱼ - rⱼ (j = 1..n)
 * Problem:
 * - select a list of targets and their starting dates sⱼ such that:
 *   - a target is observed at most once
 *   - observations do not overlap
 *   - starting dates satisfy the time-windows, i.e. rⱼ <= sⱼ and
 *     sⱼ + pⱼ <= dⱼ
 * Objective:
 * - maximize the overall profit of the selected targets
 *
 * Since 2 pⱼ ≥ dⱼ - rⱼ, targets must be scheduled by non-decreasing order of
 * rⱼ + dⱼ. Therefore, the problem can be solved in pseudo-polynomial time by
 * dynamic programming.
 * w(j, t): maximum profit of a schedule ending before t with targets 1..j.
 * w(0, t) = 0
 * w(j, t) = min w(j - 1, t - pⱼ) - wⱼ  if t ≤ dⱼ
 *               w(j - 1, t)
 *              if j != 0
 */

namespace columngenerationsolver
{

namespace singlenightstarobservationschedulingsolver
{

typedef int64_t TargetId;
typedef int64_t Profit;
typedef int64_t Time;

struct Target
{
    Time r;
    Time d;
    Time p; // Observation time.
    Profit profit;
};

class Instance
{

public:

    Instance() { }
    void add_target(const Target& target) { targets_.push_back(target); }

    virtual ~Instance() { }

    TargetId number_of_targets() const { return targets_.size(); }
    const Target& target(TargetId j) const { return targets_[j]; }

private:

    std::vector<Target> targets_;

};

struct Observation
{
    TargetId j;
    Time s;
};

class Solution
{

public:

    Solution(const Instance& instance): instance_(instance) {  }

    void add_observation(TargetId j, Time s)
    {
        observations_.push_back({j, s});
        profit_ += instance_.target(j).profit;
    }

    const Instance& instance() const { return instance_; }
    const std::vector<Observation>& observations() const { return observations_; }
    Profit profit() const { return profit_; }

private:

    const Instance& instance_;
    std::vector<Observation> observations_;
    Profit profit_ = 0;

};

inline std::ostream& operator<<(std::ostream &os, const Solution& solution)
{
    const Instance& instance = solution.instance();
    std::cout << "profit " << solution.profit() << std::endl;
    for (const Observation& o: solution.observations()) {
        os << "j " << o.j
            << " r " << instance.target(o.j).r
            << " s " << o.s
            << " e " << o.s + instance.target(o.j).p
            << " d " << instance.target(o.j).d
            << " w " << instance.target(o.j).profit
            << std::endl;
    }
    return os;
}

struct DynamicProgrammingOptionalParameters
{
    optimizationtools::Info info = optimizationtools::Info();
};

struct DynamicProgrammingState
{
    Time t;
    Profit profit;
};

inline std::ostream& operator<<(std::ostream &os, const DynamicProgrammingState& s)
{
    os << "t " << s.t << " w " << s.profit;
    return os;
}

inline Solution dynamicprogramming(
        const Instance& instance,
        DynamicProgrammingOptionalParameters parameters = {})
{
    (void)parameters;
    TargetId n = instance.number_of_targets();
    Solution solution(instance);
    //std::cout << "n " << n << std::endl;
    //for (TargetId j = 0; j < n; ++j)
    //    std::cout << "j " << j
    //        << " w " << instance.target(j).profit
    //        << " r " << instance.target(j).r
    //        << " d " << instance.target(j).d
    //        << " p " << instance.target(j).p
    //        << std::endl;

    // Sort targets.
    std::vector<TargetId> sorted_targets(n);
    std::iota(sorted_targets.begin(), sorted_targets.end(), 0);
    sort(sorted_targets.begin(), sorted_targets.end(),
            [&instance](TargetId j1, TargetId j2) -> bool
            {
                return instance.target(j1).r + instance.target(j1).d
                     < instance.target(j2).r + instance.target(j2).d;
            });

    // Compute states.
    std::vector<std::vector<DynamicProgrammingState>> states(n + 1);
    states[0].push_back({0, 0});
    for (TargetId j_pos = 0; j_pos < n; ++j_pos) {
        TargetId j = sorted_targets[j_pos];
        Time wj = instance.target(j).profit;
        Time rj = instance.target(j).r;
        Time dj = instance.target(j).d;
        Time pj = instance.target(j).p;
        //std::cout << "j_pos " << j_pos
        //    << " j " << j
        //    << " m " << (double)(rj + dj) / 2
        //    << " states[j_pos].size() " << states[j_pos].size()
        //    << std::endl;
        auto it  = states[j_pos].begin();
        auto it1 = states[j_pos].begin();
        while (it != states[j_pos].end() || it1 != states[j_pos].end()) {
            if (it1 != states[j_pos].end()
                    && (it == states[j_pos].end() || it->t > std::max(it1->t, rj) + pj)) {
                DynamicProgrammingState s1 {
                        std::max(it1->t, rj) + pj,
                        it1->profit + wj };
                //std::cout << *it1 << " -> " << s1 << std::endl;
                if (s1.t > dj) {
                    it1++;
                    continue;
                }

                if (states[j_pos + 1].empty()
                        || s1.profit > states[j_pos + 1].back().profit) {
                    if (!states[j_pos + 1].empty()
                            && s1.t == states[j_pos + 1].back().t) {
                        states[j_pos + 1].back() = s1;
                    } else {
                        states[j_pos + 1].push_back(s1);
                    }
                }
                it1++;
            } else {
                assert(it != states[j_pos].end());
                //std::cout << *it << std::endl;
                if (states[j_pos + 1].empty()
                        || it->profit > states[j_pos + 1].back().profit) {
                    if (!states[j_pos + 1].empty()
                            && it->t == states[j_pos + 1].back().t) {
                        states[j_pos + 1].back() = *it;
                    } else {
                        states[j_pos + 1].push_back(*it);
                    }
                }
                ++it;
            }
        }
    }

    // Find best state.
    DynamicProgrammingState s_best { -1, -1 };
    for (const DynamicProgrammingState& s: states[n])
        if (s_best.profit == -1 || s_best.profit < s.profit)
            s_best = s;
    //std::cout << "s_best t " << s_best.t << " w " << s_best.profit << std::endl;
    // Retrieve solution.
    DynamicProgrammingState s_curr = s_best;
    for (TargetId j_pos = n - 1; j_pos >= 0; --j_pos) {
        TargetId j = sorted_targets[j_pos];
        Time rj = instance.target(j).r;
        Time wj = instance.target(j).profit;
        Time pj = instance.target(j).p;
        if (rj > s_curr.t - pj)
            continue;
        for (const DynamicProgrammingState& s: states[j_pos]) {
            if (s.profit == s_curr.profit - wj
                    && s.t <= s_curr.t - pj) {
                solution.add_observation(j, s_curr.t - pj);
                //std::cout << "j_pos " << j_pos
                //    << " add j " << j
                //    << " s_curr " << s_curr
                //    << " s " << s
                //    << std::endl;
                s_curr = s;
                break;
            }
            if (s.profit == s_curr.profit && s.t <= s_curr.t) {
                s_curr = s;
                break;
            }
        }
    }

    //std::cout << solution << std::endl;
    return solution;
}

}

}
