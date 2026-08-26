#pragma once

#include "columngenerationsolver/commons.hpp"

#include <unordered_set>

namespace columngenerationsolver
{

struct ColumnGenerationOutput: Output
{
    /** Constructor. */
    ColumnGenerationOutput(const Model& model):
        Output(model) { }


    /** Value of the relaxation solution (with dummy columns). */
    double relaxation_solution_value = 0.0;

    /**
     * 'true' iff column generation converged to a relaxation solution
     * containing no dummy column (i.e. 'relaxation_solution' is feasible
     * for the constraints).
     *
     * Column generation proceeds in two phases per cutting-plane round: a
     * feasibility phase, with a fixed (non-escalating) dummy column weight
     * and a zeroed real objective, searching purely to eliminate dummy
     * columns; then, only once that succeeds, an optimality phase with the
     * real objective restored and no dummy columns at all. If the
     * feasibility phase still needs dummy columns once it converges, and
     * the pricing solver vouches for a genuine bound on the best
     * achievable reduced cost at that point (see
     * 'PricingSolver::PricingOutput::overcost'), that bound rigorously
     * proves infeasibility and is reported through 'bound' instead of a
     * separate flag: by the standard extended reals convention, the
     * optimal value of an infeasible problem is +inf (minimization) or
     * -inf (maximization), so 'bound' reaching that value means this
     * node/branch has no feasible solution. Without such a bound (e.g. a
     * pricing solver that never vouches for one — see the 3-way contract
     * on 'overcost'), or if it doesn't cross the infeasibility threshold,
     * the result is inconclusive rather than infeasible: 'bound' stays
     * finite and 'relaxation_solution_is_feasible' is 'false', the same
     * outcome as column generation simply running out of time or
     * iterations.
     */
    bool relaxation_solution_is_feasible = false;

    /** Number of columns in the linear subproblem. */
    ColIdx number_of_columns_in_linear_subproblem = 0;

    /** Number of times the pricing algorithm has been called. */
    Counter number_of_pricings = 0;

    Counter number_of_first_try_pricings = 0;

    Counter number_of_mispricings = 0;

    Counter number_of_no_stab_pricings = 0;

    /** Number of cutting-plane iterations. */
    Counter number_of_cutting_plane_iterations = 0;

    /**
     * The full set of cuts active at the end of this call: cuts from the
     * input cut pool that weren't removed for being inactive, plus any
     * newly separated during this call that weren't since removed either.
     * Self-contained — a caller can feed this directly as the cut pool for
     * a follow-up call without also carrying along the input cut pool
     * separately. Lives here rather than on the base 'Output' since only
     * 'column_generation' itself deals with cuts directly; algorithms
     * built on top of it (LDS, greedy) track their own node/iteration-local
     * active cut sets instead of flattening them into a whole-run list.
     */
    std::vector<std::shared_ptr<const Cut>> cuts;


    virtual int format_width() const override { return 37; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Relaxation solution is feasible: " << relaxation_solution_is_feasible << std::endl
            << std::setw(width) << std::left << "Number of pricings: " << number_of_pricings << std::endl
            << std::setw(width) << std::left << "Number of first-try pricings: " << number_of_first_try_pricings << std::endl
            << std::setw(width) << std::left << "Number of mispricings: " << number_of_mispricings << std::endl
            << std::setw(width) << std::left << "Number of no-stab pricings: " << number_of_no_stab_pricings << std::endl
            << std::setw(width) << std::left << "Number of cutting-plane iterations: " << number_of_cutting_plane_iterations << std::endl
            << std::setw(width) << std::left << "Number of cuts: " << cuts.size() << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                {"RelaxationSolutionIsFeasible", relaxation_solution_is_feasible},
                {"NumberOfPricings", number_of_pricings},
                {"NumberOfFirstTryPricings", number_of_first_try_pricings},
                {"NumberOfMispricings", number_of_mispricings},
                {"NumberOfNoStabPricings", number_of_no_stab_pricings},
                {"NumberOfCuttingPlaneIterations", number_of_cutting_plane_iterations},
                {"NumberOfCuts", cuts.size()},
                });
        return json;
    }
};

using ColumnGenerationIterationCallback = std::function<void(const ColumnGenerationOutput&)>;

/**
 * Called once at the start of each phase within a cutting-plane round
 * ('true' for the feasibility phase, 'false' for the optimality phase).
 */
using ColumnGenerationPhaseCallback = std::function<void(bool solve_feasibility)>;

/** Called once at the start of each cutting-plane round. */
using ColumnGenerationCuttingPlaneCallback = std::function<void(Counter cutting_plane_iteration)>;

struct ColumnGenerationParameters: Parameters
{
    /** Linear programming solver. */
    SolverName solver_name = SolverName::CLP;

    /**
     * Maximum number of iterations.
     *
     * This budget is shared across both the feasibility and optimality
     * phases within one 'column_generation()' call (see
     * 'ColumnGenerationOutput::relaxation_solution_is_feasible'), so an
     * expensive feasibility phase could in principle starve the
     * optimality phase of iterations during a tightly capped call (e.g.
     * branch-and-price's strong-branching evaluation). This degrades
     * candidate scoring but doesn't break correctness -- the "clamp to
     * parent's bound" safety net in 'branch_and_price' still holds.
     */
    Counter maximum_number_of_iterations = -1;

    /** Maximum number of cutting-plane iterations. */
    Counter maximum_number_of_cutting_plane_iterations = -1;

    /**
     * Tolerance for the reduced cost optimality check.
     *
     * A column is only added when its reduced cost is strictly below
     * -optimality_tolerance (minimization) or above +optimality_tolerance
     * (maximization), guarding against LP dual imprecision (~1e-7).
     */
    Value optimality_tolerance = 0.0;

    /** Callback function called at each column generation iteration. */
    ColumnGenerationIterationCallback iteration_callback = [](const Output&) { };

    /** Callback function called at the start of each phase. */
    ColumnGenerationPhaseCallback phase_callback = [](bool) { };

    /** Callback function called at the start of each cutting-plane round. */
    ColumnGenerationCuttingPlaneCallback cutting_plane_callback = [](Counter) { };

    /*
     * Stabilization parameters
     */

    /** Static Wentges smoothing parameter (alpha). */
    Value static_wentges_smoothing_parameter = 0;

    /** Enable self-adjusting Wentges smoothing. */
    bool self_adjusting_wentges_smoothing = false;

    /** Static directional smoothing parameter (beta). */
    Value static_directional_smoothing_parameter = 0.0;

    /** Enable automatic directional smoothing. */
    bool automatic_directional_smoothing = false;


    /**
     * Tabu columns.
     *
     * These columns are never added to the LP solver and therefore won't be
     * part of the returned solution.
     */
    std::unordered_set<std::shared_ptr<const Column>> tabu;

    /**
     * Fraction of the starting infeasibility that must remain before the
     * rounding heuristic's greedy fixing phase stops and hands off to its
     * completion phase (e.g. 0.2 means "stop once 80% has been resolved").
     * If the greedy phase exhausts all relaxation columns without reaching
     * this, the heuristic gives up for this iteration without ever calling
     * pricing.
     */
    Value rounding_heuristic_infeasibility_threshold = 0.2;


    virtual int format_width() const override { return 45; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Linear programming solver: " << solver_name << std::endl
            << std::setw(width) << std::left << "Static Wentges smoothing parameter: " << static_wentges_smoothing_parameter << std::endl
            << std::setw(width) << std::left << "Static directional smoothing parameter: " << static_directional_smoothing_parameter << std::endl
            << std::setw(width) << std::left << "Self-adjusting Wentges smoothing: " << self_adjusting_wentges_smoothing << std::endl
            << std::setw(width) << std::left << "Automatic directional smoothing: " << automatic_directional_smoothing << std::endl
            << std::setw(width) << std::left << "Maximum number of iterations: " << maximum_number_of_iterations << std::endl
            << std::setw(width) << std::left << "Maximum number of cutting-plane iterations: " << maximum_number_of_cutting_plane_iterations << std::endl
            << std::setw(width) << std::left << "Optimality tolerance: " << optimality_tolerance << std::endl
            << std::setw(width) << std::left << "Tabu size: " << tabu.size() << std::endl
            << std::setw(width) << std::left << "Rounding heuristic infeasibility threshold: " << rounding_heuristic_infeasibility_threshold << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                {"SolverName", solver_name},
                {"StaticWentgesSmoothingParameter", static_wentges_smoothing_parameter},
                {"StaticDirectionalSmoothingParameter", static_directional_smoothing_parameter},
                {"SelfAdjustingWentgesSmoothing", self_adjusting_wentges_smoothing},
                {"AutomaticDirectionalSmoothing", automatic_directional_smoothing},
                {"MaximumNumberOfIterations", maximum_number_of_iterations},
                {"MaximumNumberOfCuttingPlaneIterations", maximum_number_of_cutting_plane_iterations},
                {"OptimalityTolerance", optimality_tolerance},
                {"TabuSize", tabu.size()},
                {"RoundingHeuristicInfeasibilityThreshold", rounding_heuristic_infeasibility_threshold},
                });
        return json;
    }
};

const ColumnGenerationOutput column_generation(
        const Model& model,
        const ColumnGenerationParameters& parameters = {});

}
