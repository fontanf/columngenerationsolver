# Column generation

`--algorithm column-generation`, `columngenerationsolver::column_generation(model, parameters)` (`include/columngenerationsolver/algorithms/column_generation.hpp`, `src/algorithms/column_generation.cpp`).

Solves the linear relaxation of the master problem of a Dantzig–Wolfe decomposition by column generation, and computes a dual bound. It is also the building block of the other algorithms of the package (`greedy`, `limited-discrepancy-search`, `branch-and-price`), which call it at each node with fixed columns and branching decisions.

## Model

The master problem is a linear program over columns (minimization, or maximization with the opposite inequalities):

$$\min \sum_j c_j x_j \quad \text{s.t.} \quad l_r \le \sum_j a_{rj} x_j \le u_r \ \ \forall r, \qquad lb_j \le x_j \le ub_j$$

* `Model::rows`: the rows $r$, with their bounds $l_r$ and $u_r$ (possibly infinite, equal for an equality row), and the bounds of the coefficients of the columns to generate in each row.
* `Model::static_columns`: columns known in advance, with their own bounds $[lb_j, ub_j]$.
* Generated columns: returned by the pricing solver (`Model::pricing_solver`, a `PricingSolver`), always with bounds $[0, +\infty)$. A column has an objective coefficient $c_j$ and its coefficients $a_{rj}$ (`Column::elements`).

Given duals $\pi$, the reduced cost of a column is $\bar c_j(\pi) = c_j - \sum_r \pi_r a_{rj}$ (`PricingSolver::compute_reduced_cost`; cuts add their own dual terms). The pricing solver searches for columns of negative reduced cost (positive when maximizing) given duals (`PricingSolver::solve_pricing`).

The columns fixed by the caller (`Parameters::fixed_columns`, e.g. by diving) are removed from the master problem: their cost is a constant $c_0$, and the bounds of the rows become residual bounds. The rows already satisfied by the fixed columns are removed.

## Algorithm

Each iteration:
1. Solve the restricted master LP (the columns generated so far); its value is the relaxation value, and its duals are $\pi^{\text{out}}$.
2. Search the column pool (columns generated before, in this call or given by the caller) for columns of negative reduced cost at $\pi^{\text{out}}$.
3. If none, call the pricing solver, possibly several times with stabilized duals (see Stabilization).
4. Update the dual bound after each pricing call (see Dual bound).
5. Stop if no new column of negative reduced cost at $\pi^{\text{out}}$ has been found (the relaxation is solved), if the dual bound proves optimality or infeasibility, if it reaches `objective_cutoff`, or at the time or iteration limit. Otherwise, add the new columns to the LP.

Columns identical to a column already generated are never added twice. Tabu columns (`ColumnGenerationParameters::tabu`) are not added.

### Two phases

Each attempt has two phases:
* Feasibility phase: the objective coefficients of the real columns are set to 0, and a dummy column of cost 1 is added for each row (and cut) that the fixed, static and initial columns can't satisfy. The pricing solver is called with `solve_feasibility = true`, i.e. with zero objective coefficients. The phase stops as soon as the LP doesn't use any dummy column (checked on a small LP restricted to the columns used, since a dummy column can be used with a value below the tolerance).
* Optimality phase: the real objective, no dummy column, warm-started from the columns of the feasibility phase.

If dummy columns remain when the feasibility phase converges, the result is either a proof of infeasibility (`bound` is $+\infty$ when minimizing, $-\infty$ when maximizing; see Dual bound) or inconclusive (`relaxation_solution_is_feasible` is `false`, as with a time limit).

### Dual pricing

A pricing solver can implement a second, more expensive pricing (`PricingType::Dual`, enabled by `PricingSolver::has_pricing_bound`), for example an exact algorithm behind a heuristic `PricingType::Primal` pricing. Once the primal pricing has converged, it is called (at most twice per call to `column_generation`, at $\pi^{\text{out}}$) only if succeeding could cut the node: in the feasibility phase if dummy columns remain, in the optimality phase if the relaxation value already reaches `objective_cutoff`. `overcost_goal` tells it which `overcost` is enough. If it finds new columns, column generation continues.

### Cutting planes

With `cutting_planes`, once the relaxation is solved, cuts are separated (`PricingSolver::separate_cuts`, with a pool of the last separated cuts), and the most violated one is added. The master LP is then rebuilt with the new cut set, and the two phases are run again. Cuts that have become stale are removed. Cuts have their own duals, which are not stabilized.

### Primal heuristics

* Rounding heuristic (`rounding_heuristic`): at each iteration of the optimality phase where the pricing solver has been called, greedily fixes columns of the relaxation, then completes the solution by alternating pricing and fixing, without modifying the master LP.
* Internal diving (`internal_diving`): the pricing solver is called repeatedly, fixing the columns it returns, so that it returns columns that fit together (Sadykov et al., 2019).

## Stabilization

The duals of the restricted master LP oscillate, especially during the first iterations. The pricing solver can be called at stabilized duals $\pi^{\text{sep}}$ (the separation point) instead of $\pi^{\text{out}}$:

* Wentges smoothing (`static_wentges_smoothing_parameter` $\alpha$): $\pi^{\text{sep}} = \alpha \pi^{\text{in}} + (1 - \alpha) \pi^{\text{out}}$, where $\pi^{\text{in}}$ is the stability center: by default, the separation point of the previous pricing call; with `wentges_smoothing_best_bound_center`, the duals of the best Lagrangian bound of the attempt, as in Wentges (1997). The latter requires a pricing solver which returns an `overcost` (see Dual bound); until a bound is known, and in the feasibility phase, the previous separation point is used.
* Self-adjusting Wentges smoothing (`self_adjusting_wentges_smoothing`): $\alpha$ is decreased by 0.1 if the subgradient $g$ at the separation point satisfies $g \cdot (\pi^{\text{out}} - \pi^{\text{in}}) > 0$, and increased by $0.1 (1 - \alpha)$ otherwise (at most 0.99) (Pessoa et al., 2018).
* Directional smoothing (`static_directional_smoothing_parameter` $\beta$, or `automatic_directional_smoothing`): the separation point is moved towards the subgradient direction, $\pi^{g} = \pi^{\text{in}} + \frac{\|\pi^{\text{out}} - \pi^{\text{in}}\|}{\|g\|} g$, $\rho = \beta \pi^{g} + (1 - \beta) \pi^{\text{out}}$, and $\pi^{\text{sep}}$ is the point of the segment from $\pi^{\text{in}}$ to $\rho$ at the distance of $\alpha \pi^{\text{in}} + (1 - \alpha) \pi^{\text{out}}$ from $\pi^{\text{in}}$. With `automatic_directional_smoothing`, $\beta$ is the cosine of the angle between $\pi^{\text{out}} - \pi^{\text{in}}$ and $\pi^{g} - \pi^{\text{in}}$, clamped to $[0, 1]$ (Pessoa et al., 2018).

The subgradient at the separation point is $g_r = \min(0, u_r - y_r) + \max(0, l_r - y_r)$, where $y_r$ is the value of row $r$ for the columns returned by the pricing solver (`PricingOutput::lagrangian_column_values`, if the pricing solver gives them: e.g. $N$ times the coefficients of the column for $N$ identical subproblems).

Mispricing: if the pricing solver finds no column of negative reduced cost at $\pi^{\text{out}}$ at the separation point, it is called again with $\alpha_k = \max(0, 1 - k (1 - \alpha))$ at the $k$-th try (and without directional smoothing for $k > 1$), until a column is found or $\alpha_k = 0$, i.e. $\pi^{\text{sep}} = \pi^{\text{out}}$. Column generation only stops after a pricing call at $\pi^{\text{out}}$ which finds no column, so smoothing doesn't change the value of the relaxation, only the number of iterations.

## Dual bound

### The overcost

The pricing solver gives a bound on the reduced costs of all the columns it could generate, not only the ones it returns: `PricingOutput::overcost`, computed at the duals of the call. More precisely, `overcost` is a lower bound (minimization) on $\sum_{j \text{ generated}} \bar c_j(\pi) x_j$ over all the feasible solutions $x$, which accounts for the number of columns of each subproblem in a solution:
* one column per subproblem (e.g. a single machine, or one column of each type): the smallest reduced cost (or 0 if the subproblem can be empty);
* $N$ identical subproblems (e.g. $N$ bins): $N \min(0, \bar c^*)$, as in the cutting stock example (with $N$ the total demand).

Three behaviors are possible:
* exact pricing: `overcost` is the optimal reduced cost (times the multiplicity), whether it's negative or not;
* heuristic pricing without bound: `overcost` stays $+\infty$, and the call doesn't give a bound;
* heuristic pricing with a bound: `overcost` is a bound obtained by other means, e.g. a relaxation of the pricing problem.

With `solve_feasibility = true`, `overcost` bounds the reduced costs computed with zero objective coefficients.

### The Lagrangian bound

Relaxing the rows of the master problem with duals $\pi$ gives the Lagrangian bound. For any feasible solution $x$ (minimization):

$$\sum_j c_j x_j = \sum_j \bar c_j(\pi) x_j + \sum_r \pi_r \sum_j a_{rj} x_j$$

and each term is bounded:
* row $r$: $\pi_r \sum_j a_{rj} x_j \ge \pi_r l_r$ if $\pi_r > 0$, and $\ge \pi_r u_r$ if $\pi_r < 0$ (no finite bound if this bound of the row is infinite; any sign for an equality row);
* cut: the same with its own dual and bounds;
* static column $j$: $\bar c_j x_j \ge \bar c_j\, lb_j$ if $\bar c_j > 0$, and $\ge \bar c_j\, ub_j$ if $\bar c_j < 0$ (no finite bound if this bound is infinite);
* generated columns: $\sum_j \bar c_j(\pi) x_j \ge$ `overcost`, by definition.

So, with $c_0$ the cost of the fixed columns and the residual bounds of the rows:

$$L(\pi) = c_0 + \sum_r \pi_r \begin{cases} l_r & \pi_r > 0 \\ u_r & \pi_r < 0 \end{cases} + \sum_{\text{cuts}} (\dots) + \sum_{j \text{ static}} \bar c_j(\pi) \begin{cases} lb_j & \bar c_j > 0 \\ ub_j & \bar c_j < 0 \end{cases} + \text{overcost}(\pi)$$

is a lower bound of the master problem (and of the problem, if the master problem is a relaxation of it) at any duals $\pi$ of the right signs: $\pi_r \ge 0$ for a row $\sum_j a_{rj} x_j \ge l_r$, $\le 0$ for a row $\le u_r$. When maximizing, the inequalities are reversed, and $L(\pi)$ is an upper bound. $L$ is the Lagrangian function of the master problem: it is concave, and its maximum is the value of the linear relaxation. Column generation can be seen as a method that maximizes it.

### At the duals of the master LP

At the optimal duals $\pi^{\text{out}}$ of the restricted master LP, by LP duality and complementary slackness, the value of the LP is

$$z_{\text{RMP}} = c_0 + \sum_r (\text{row terms at } \pi^{\text{out}}) + \sum_{j \text{ static}} (\text{column terms at } \pi^{\text{out}})$$

since the generated columns of the LP have a zero contribution (a reduced cost of 0 if they are used, a value of 0 otherwise). So the bound is computed without the row and column terms:

$$L(\pi^{\text{out}}) = z_{\text{RMP}} + \text{overcost}(\pi^{\text{out}})$$

It is the usual Lagrangian bound of column generation (see Lübbecke and Desrosiers, 2005).

### At the stabilized duals

With smoothing, the pricing solver is called at $\pi^{\text{sep}} \ne \pi^{\text{out}}$, and its `overcost` is computed at $\pi^{\text{sep}}$. Then $z_{\text{RMP}} + \text{overcost}(\pi^{\text{sep}})$ is not a valid bound: $z_{\text{RMP}}$ is the value of the row and column terms at $\pi^{\text{out}}$, not at $\pi^{\text{sep}}$ (for equality rows, the difference is $(\pi^{\text{out}} - \pi^{\text{sep}}) \cdot b$, which can be positive). So the algorithm computes $L(\pi^{\text{sep}})$ explicitly, from the row, cut and static column terms at $\pi^{\text{sep}}$. $\pi^{\text{sep}}$ is a convex combination of duals of the right signs, so it has the right signs.

In practice (`apply_overcost`):
* the bound is updated after each pricing call, at the duals of the call, including each call of the mispricing loop: $z_{\text{RMP}} + \text{overcost}$ if the duals are those of the master LP, and $L(\pi)$ computed explicitly otherwise;
* a pricing call without bound (`overcost` infinite) doesn't update it;
* `Output::bound` is the best of these bounds;
* at convergence, the last pricing call is at $\pi^{\text{out}}$ and finds no column of negative reduced cost; with an exact pricing, its bound is then the value of the relaxation.

### Infeasibility proof

In the feasibility phase, the objective of the LP is the cost of the dummy columns. If dummy columns remain when the phase converges, the value of the LP plus `overcost` (computed with zero objective coefficients) bounds the cost of the dummy columns in any solution: if it is positive (beyond the tolerance), no solution without dummy columns exists, and `bound` is set to $+\infty$ (minimization) or $-\infty$ (maximization). This proof is only used with an `overcost` computed at the duals of the master LP, since the dummy columns are not part of the problem and $L(\pi)$ is not computed for them.

## Parameters

| Parameter | Default | Description |
| --- | --- | --- |
| `solver_name` | CLP | LP solver: CLP, HiGHS, Xpress, CPLEX or Knitro (depending on the build) |
| `maximum_number_of_iterations` | -1 (no limit) | Iterations, shared by both phases |
| `maximum_number_of_cutting_plane_iterations` | -1 | Cutting-plane rounds |
| `objective_cutoff` | $+\infty$ (none) | Stop as soon as the bound shows that no solution strictly better than this value exists |
| `optimality_tolerance` | 0 | A column is added if its reduced cost is below $-$tolerance (minimization) |
| `static_wentges_smoothing_parameter` | 0 | $\alpha$ |
| `self_adjusting_wentges_smoothing` | false | Adjust $\alpha$ during the algorithm |
| `wentges_smoothing_best_bound_center` | false | Stability center at the duals of the best Lagrangian bound instead of the previous separation point |
| `static_directional_smoothing_parameter` | 0 | $\beta$ |
| `automatic_directional_smoothing` | false | Compute $\beta$ at each iteration |
| `column_pool`, `initial_columns`, `fixed_columns`, `branching_decisions`, `initial_cuts`, `tabu` | empty | Known columns, warm start, node of a tree search |
| `internal_diving`, `rounding_heuristic`, `cutting_planes` | Never | Never, at the first opportunity only (`Initial`), or always |

## Output

* `bound`: the dual bound (see Dual bound).
* `relaxation_solution`, `relaxation_solution_value`, `relaxation_solution_is_feasible`: the solution of the restricted master LP at the end, and whether it doesn't use dummy columns.
* `solution`: a feasible solution, if the rounding heuristic found one.
* `columns`: the generated columns; `cuts`: the active cuts at the end.
* Counters: iterations, pricings, mispricings (pricing calls after the first try of an iteration), pricings without stabilization, dual pricings, cutting-plane rounds; and the times of the LP, of the pricing, of the column pool search, of the separation and of the rounding heuristic.

## References

* P. Wentges (1997). Weighted Dantzig–Wolfe decomposition for linear mixed-integer programming. *International Transactions in Operational Research*, 4(2), 151–162. [doi:10.1111/j.1475-3995.1997.tb00071.x](https://doi.org/10.1111/j.1475-3995.1997.tb00071.x)
* M. E. Lübbecke and J. Desrosiers (2005). Selected topics in column generation. *Operations Research*, 53(6), 1007–1023. [doi:10.1287/opre.1050.0234](https://doi.org/10.1287/opre.1050.0234)
* A. Pessoa, R. Sadykov, E. Uchoa and F. Vanderbeck (2018). Automation and combination of linear-programming based stabilization techniques in column generation. *INFORMS Journal on Computing*, 30(2), 339–360. [doi:10.1287/ijoc.2017.0784](https://doi.org/10.1287/ijoc.2017.0784)
* R. Sadykov, F. Vanderbeck, A. Pessoa, I. Tahiri and E. Uchoa (2019). Primal heuristics for branch and price: the assets of diving methods. *INFORMS Journal on Computing*, 31(2), 251–267. [doi:10.1287/ijoc.2018.0822](https://doi.org/10.1287/ijoc.2018.0822)
