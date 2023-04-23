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

#pragma once

#include "optimizationtools/utils/info.hpp"

namespace columngenerationsolver
{

namespace singlenightstarobservationschedulingsolver
{

typedef int64_t TargetId;
typedef double Profit;
typedef int64_t Time;

/**
 * Structure for a target.
 */
struct Target
{
    /** Release date. */
    Time release_date;

    /** Deadline. */
    Time deadline;

    /** Observation time. */
    Time observation_time;

    /** Profit; */
    Profit profit;
};

/**
 * Instance class for a 'singlenightstarobservationschedulingsolver' problem.
 */
class Instance
{

public:

    /*
     * Constructors and destructor
     */

    /** Constructor to build an instance manually. */
    Instance() { }

    /** Add a target. */
    void add_target(
            Time release_date,
            Time deadline,
            Time observation_time,
            Profit profit)
    {
        Target target;
        target.release_date = release_date;
        target.deadline = deadline;
        target.observation_time = observation_time;
        target.profit = profit;
        targets_.push_back(target);
    }

    /*
     * Getters
     */

    /** Get the number of targets. */
    TargetId number_of_targets() const { return targets_.size(); }

    /** Get a target. */
    const Target& target(TargetId target_id) const { return targets_[target_id]; }

private:

    /*
     * Private attributes
     */

    /** Targets. */
    std::vector<Target> targets_;

};

/**
 * Structure for an observation.
 */
struct Observation
{
    /** Target. */
    TargetId target_id;

    /** Start time. */
    Time start_time;
};

/**
 * Solution class for a 'singlenightstarobservationschedulingsolver' problem.
 */
class Solution
{

public:

    /*
     * Constructors and destructor
     */

    /** Constructor. */
    Solution(const Instance& instance): instance_(&instance) {  }

    /** Add an observation. */
    void add_observation(
            TargetId target_id,
            Time start_time)
    {
        const Target& target = instance().target(target_id);
        if (target.release_date > start_time) {
            throw std::runtime_error(
                    "target.release_date > start_time");
        }
        if (target.deadline < start_time + target.observation_time) {
            std::cout << "target_id " << target_id << std::endl;
            std::cout << "release_date " << target.release_date << std::endl;
            std::cout << "deadline " << target.deadline << std::endl;
            std::cout << "observation_time " << target.observation_time << std::endl;
            std::cout << "start_time " << start_time << std::endl;
            throw std::runtime_error(
                    "target.deadline < start_time + target.observation_time");
        }

        Observation observation;
        observation.target_id = target_id;
        observation.start_time = start_time;
        observations_.push_back(observation);
        profit_ += instance().target(target_id).profit;
    }

    /** Get the instance. */
    const Instance& instance() const { return *instance_; }

    /** Get the number of observations. */
    TargetId number_of_observations() const { return observations_.size(); }

    /** Get an observation. */
    const Observation& observation(TargetId observation_pos) const { return observations_[observation_pos]; }

    /** Get the profit of the solution. */
    Profit profit() const { return profit_; }

private:

    /*
     * Private attributes
     */

    /** Instance. */
    const Instance* instance_;

    /** Observations. */
    std::vector<Observation> observations_;

    /** Profit of the solution. */
    Profit profit_ = 0;

};

struct DynamicProgrammingOptionalParameters
{
    optimizationtools::Info info = optimizationtools::Info();
};

struct DynamicProgrammingState
{
    Time time;
    Profit profit;
    TargetId target_id;
    DynamicProgrammingState* prev;
};

inline std::ostream& operator<<(std::ostream &os, const DynamicProgrammingState& s)
{
    os << "time " << s.time << " profit " << s.profit;
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
            [&instance](TargetId target_id_1, TargetId target_id_2) -> bool
            {
                return instance.target(target_id_1).release_date
                    + instance.target(target_id_1).deadline
                    < instance.target(target_id_2).release_date
                    + instance.target(target_id_2).deadline;
            });

    // Compute states.
    std::vector<std::vector<DynamicProgrammingState>> states(n + 1);
    states[0].push_back({0, 0, -1, nullptr});
    for (TargetId target_pos = 0; target_pos < n; ++target_pos) {
        TargetId target_id = sorted_targets[target_pos];
        const Target& target = instance.target(target_id);
        //std::cout << "target_pos " << target_pos
        //    << " j " << j
        //    << " m " << (double)(rj + dj) / 2
        //    << " states[target_pos].size() " << states[target_pos].size()
        //    << std::endl;
        auto it  = states[target_pos].begin();
        auto it1 = states[target_pos].begin();
        while (it != states[target_pos].end() || it1 != states[target_pos].end()) {
            if (it1 != states[target_pos].end()
                    && (it == states[target_pos].end()
                        || it->time > std::max(it1->time, target.release_date)
                        + target.observation_time)) {
                DynamicProgrammingState s1 {
                        std::max(it1->time, target.release_date) + target.observation_time,
                        it1->profit + target.profit,
                        target_id,
                        &(*it1) };
                //std::cout << *it1 << " -> " << s1 << std::endl;
                if (s1.time > target.deadline) {
                    it1++;
                    continue;
                }

                if (states[target_pos + 1].empty()
                        || s1.profit > states[target_pos + 1].back().profit) {
                    if (!states[target_pos + 1].empty()
                            && s1.time == states[target_pos + 1].back().time) {
                        states[target_pos + 1].back() = s1;
                    } else {
                        states[target_pos + 1].push_back(s1);
                    }
                }
                it1++;
            } else {
                assert(it != states[target_pos].end());
                //std::cout << *it << std::endl;
                if (states[target_pos + 1].empty()
                        || it->profit > states[target_pos + 1].back().profit) {
                    if (!states[target_pos + 1].empty()
                            && it->time == states[target_pos + 1].back().time) {
                        states[target_pos + 1].back() = *it;
                    } else {
                        states[target_pos + 1].push_back(*it);
                    }
                }
                ++it;
            }
        }
    }

    // Find best state.
    const DynamicProgrammingState* s_best = nullptr;
    for (const DynamicProgrammingState& s: states[n])
        if (s_best == nullptr || s_best->profit < s.profit)
            s_best = &s;
    //std::cout << "s_best t " << s_best.time << " profit " << s_best.profit << std::endl;
    // Retrieve solution.
    const DynamicProgrammingState* s_curr = s_best;
    while (s_curr->prev != nullptr) {
        solution.add_observation(
                s_curr->target_id,
                s_curr->time - instance.target(s_curr->target_id).observation_time);
        s_curr = s_curr->prev;
    }
    return solution;
}

}

}
