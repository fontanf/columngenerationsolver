#pragma once

#include "optimizationtools/info.hpp"
#include "optimizationtools/utils.hpp"
#include "optimizationtools/sorted_on_demand_array.hpp"

/**
 * Single machine order acceptance and scheduling problem with family setup
 * times, Total weighted completion time.
 *
 * Input:
 * - o families with setup time sₖ (k = 1..o)
 * - n jobs with (j = 1..n)
 *   - a processing time pⱼ
 *   - a weight vⱼ
 *   - a family fⱼ
 *   - a profit vⱼ
 * Problem:
 * - find a schedule such that:
 *   - each job is scheduled at most once
 *   - if a job j2 is processed immediately after a job j1 and f(j2) != f(j1),
 *     then a setup of s(f(j2)) units of time must be performed before starting
 *     processing j2.
 * Objective:
 * - minimize the total weighted completion time of the schedule minus the
 *   total profit of the schedule.
 *
 * Tree search:
 * - forward branching
 * - guide: objective
 *
 */

namespace columngenerationsolver
{

namespace oaschedulingwithfamilysetuptimestwct
{

typedef int64_t JobId;
typedef int64_t JobPos;
typedef int64_t MachineId;
typedef int64_t FamilyId;
typedef double Time;
typedef double Weight;
typedef double Profit;

struct Family
{
    FamilyId id;
    Time setup_time;
    std::vector<JobId> jobs;
};

struct Job
{
    JobId id;
    Time processing_time;
    Weight weight;
    FamilyId family;
    Profit profit;
};

class Instance
{

public:

    Instance(FamilyId number_of_familiess):
        families_(number_of_familiess)
    {
        for (FamilyId k = 0; k < number_of_familiess; ++k)
            families_[k].id = k;
    }
    void set_setup_time(FamilyId k, Time setup_time) { families_[k].setup_time = setup_time; }
    void add_job(
            Time processing_time,
            Weight weight,
            FamilyId family,
            Profit profit)
    {
        JobId id = jobs_.size();
        Job job;
        job.id = id;
        job.processing_time = processing_time;
        job.weight = weight;
        job.family = family;
        job.profit = profit;
        jobs_.push_back(job);
        families_[family].jobs.push_back(id);
    }

    virtual ~Instance() { }

    FamilyId number_of_familiess() const { return families_.size(); }
    JobId number_of_jobs() const { return jobs_.size(); }
    const Job& job(JobId j) const { return jobs_[j]; }
    const Family& family(FamilyId k) const { return families_[k]; }

private:

    std::vector<Job> jobs_;
    std::vector<Family> families_;

};

static std::ostream& operator<<(
        std::ostream &os, const Instance& instance)
{
    os << "job number " << instance.number_of_jobs() << std::endl;
    os << "family number " << instance.number_of_familiess() << std::endl;
    for (JobId j = 0; j < instance.number_of_jobs(); ++j)
        os << "job " << j
            << " p " << instance.job(j).processing_time
            << " w " << instance.job(j).weight
            << " p " << instance.job(j).profit
            << " f " << instance.job(j).family
            << std::endl;
    for (FamilyId k = 0; k < instance.number_of_familiess(); ++k) {
        os << "family " << k
            << " s " << instance.family(k).setup_time
            << " j";
        for (JobId j: instance.family(k).jobs)
            os << " " << j;
        os << std::endl;
    }
    return os;
}

class BranchingScheme
{

public:

    struct Node
    {
        std::shared_ptr<Node> father = nullptr;
        std::vector<bool> available_jobs;
        JobId j = -1;
        JobId number_of_jobs = 0;
        Time time = 0;
        Weight total_weighted_completion_time = 0;
        Profit profit = 0;
        double guide = 0;
        JobPos next_child_pos = 0;
    };

    BranchingScheme(const Instance& instance):
        instance_(instance),
        sorted_jobs_(instance.number_of_jobs() + 1),
        generator_(0)
    {
        // Initialize sorted_jobs_.
        JobId n = instance_.number_of_jobs();
        sorted_jobs_[n].reset(instance.number_of_jobs());
        for (JobId j = 0; j < n; ++j) {
            Time p = instance_.job(j).processing_time;
            Weight w = instance_.job(j).weight;
            FamilyId k = instance_.job(j).family;
            Time s = instance_.family(k).setup_time;
            double r = p / w;
            sorted_jobs_[n].set_cost(j, (s + p) / w);
            sorted_jobs_[j].reset(instance.number_of_jobs());
            for (JobId j2 = 0; j2 < n; ++j2) {
                Time p2 = instance_.job(j2).processing_time;
                Weight w2 = instance_.job(j2).weight;
                FamilyId k2 = instance_.job(j2).family;
                Time s2 = instance_.family(k2).setup_time;
                if (j2 == j) {
                    sorted_jobs_[j].set_cost(j2, std::numeric_limits<double>::max());
                } else if (k2 == k) {
                    double r2 = p2 / w2;
                    if (r < r2 || (r == r2 && j < j2)) {
                        sorted_jobs_[j].set_cost(j2, r2);
                    } else {
                        sorted_jobs_[j].set_cost(j2, std::numeric_limits<double>::max());
                    }
                } else {
                    sorted_jobs_[j].set_cost(j2, (s2 + p2) / w2);
                }
            }
        }
    }

    inline JobId neighbor(JobId j, JobPos pos) const
    {
        assert(j >= 0);
        assert(j <= instance_.number_of_jobs());
        assert(pos >= 0);
        assert(pos < instance_.number_of_jobs());
        return sorted_jobs_[j].get(pos, generator_);
    }

    inline const std::shared_ptr<Node> root() const
    {
        auto r = std::shared_ptr<Node>(new BranchingScheme::Node());
        r->j = instance_.number_of_jobs();
        r->available_jobs.resize(instance_.number_of_jobs(), true);
        return r;
    }

    inline std::shared_ptr<Node> next_child(
            const std::shared_ptr<Node>& father) const
    {
        assert(!infertile(father));
        assert(!leaf(father));
        JobId j_next = neighbor(father->j, father->next_child_pos);
        // Update father
        father->next_child_pos++;
        if (!father->available_jobs[j_next])
            return nullptr;
        FamilyId k_next = instance_.job(j_next).family;
        Time p_next = instance_.job(j_next).processing_time;
        Weight w_next = instance_.job(j_next).weight;
        Profit v_next = instance_.job(j_next).profit;
        Time t = father->time + p_next;
        if (father->j == instance_.number_of_jobs()
                || k_next != instance_.job(father->j).family)
            t += instance_.family(k_next).setup_time;
        if (w_next * t >= v_next)
            return nullptr;

        // Compute new child.
        auto child = std::shared_ptr<Node>(new BranchingScheme::Node());
        child->father = father;
        child->available_jobs = father->available_jobs;
        child->available_jobs[j_next] = false;
        child->j = j_next;
        child->number_of_jobs = father->number_of_jobs + 1;
        child->time = t;
        child->total_weighted_completion_time = father->total_weighted_completion_time + w_next * t;
        child->profit = father->profit + v_next;
        child->guide = child->time / (child->profit - child->total_weighted_completion_time);
        // Within a family, it is dominant to follow WSPT order.
        // Therefore, we discard all jobs of the same family with a smaller
        // ratio pj / wj.
        double r_next = p_next / w_next;
        for (JobId j: instance_.family(k_next).jobs) {
            if (!child->available_jobs[j])
                continue;
            double r = instance_.job(j).processing_time / instance_.job(j).weight;
            if (r < r_next || (r == r_next && j < j_next))
                child->available_jobs[j] = false;
        }
        return child;
    }

    inline bool infertile(
            const std::shared_ptr<Node>& node) const
    {
        assert(node != nullptr);
        return (node->next_child_pos == instance_.number_of_jobs());
    }

    inline bool operator()(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        assert(!infertile(node_1));
        assert(!infertile(node_2));
        if (node_1->guide != node_2->guide)
            return node_1->guide < node_2->guide;
        return node_1.get() < node_2.get();
    }

    inline bool leaf(
            const std::shared_ptr<Node>& node) const
    {
        return node->number_of_jobs == instance_.number_of_jobs();
    }

    bool bound(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        (void)node_1;
        (void)node_2;
        return false;
    }

    bool better(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        return node_1->total_weighted_completion_time - node_1->profit
            < node_2->total_weighted_completion_time - node_2->profit;
    }

    bool equals(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_jobs != node_2->number_of_jobs)
            return false;
        std::vector<bool> v(instance_.number_of_jobs(), false);
        for (auto node_tmp = node_1; node_tmp->father != nullptr; node_tmp = node_tmp->father)
            v[node_tmp->j] = true;
        for (auto node_tmp = node_1; node_tmp->father != nullptr; node_tmp = node_tmp->father)
            if (!v[node_tmp->j])
                return false;
        return true;
    }

    std::string display(const std::shared_ptr<Node>& node) const
    {
        if (node->j == 0)
            return "";
        std::stringstream ss;
        ss << node->total_weighted_completion_time - node->profit
            << " (n" << node->number_of_jobs
            << " t" << node->time
            << " twct" << node->total_weighted_completion_time
            << " v" << node->profit
            << ")";
        return ss.str();
    }

    /**
     * Dominances.
     */

    inline bool comparable(
            const std::shared_ptr<Node>& node) const
    {
        (void)node;
        return true;
    }

    struct NodeHasher
    {
        std::hash<JobId> hasher_1;
        std::hash<std::vector<bool>> hasher_2;
        const Instance instance;

        NodeHasher(const Instance& instance): instance(instance) { }

        inline bool operator()(
                const std::shared_ptr<Node>& node_1,
                const std::shared_ptr<Node>& node_2) const
        {
            if (instance.job(node_1->j).family != instance.job(node_2->j).family)
                return false;
            if (node_1->available_jobs != node_2->available_jobs)
                return false;
            return true;
        }

        inline std::size_t operator()(
                const std::shared_ptr<Node>& node) const
        {
            size_t hash = hasher_1(instance.job(node->j).family);
            optimizationtools::hash_combine(hash, hasher_2(node->available_jobs));
            return hash;
        }
    };

    inline NodeHasher node_hasher() const { return NodeHasher(instance_); }

    inline bool dominates(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->total_weighted_completion_time - node_1->profit
                <= node_2->total_weighted_completion_time - node_2->profit
                && node_1->time <= node_2->time)
            return true;
        return false;
    }

private:

    const Instance& instance_;

    mutable std::vector<optimizationtools::SortedOnDemandArray> sorted_jobs_;
    mutable std::mt19937_64 generator_;

};

}

}

