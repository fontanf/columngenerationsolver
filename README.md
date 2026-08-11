# ColumnGenerationSolver

A solver based on column generation.

![columngeneration](img/columngeneration.jpg?raw=true "columngeneration")

[image source](https://commons.wikimedia.org/wiki/File:ColonnesPavillonTrajan.jpg)

## Description

The goal of this package is to provide a simple framework to quickly implement algorithms based on column generation.

To get a working heuristic based on the algorithms from "Primal Heuristics for Branch and Price: The Assets of Diving Methods" (Sadykov et al., 2019), it is only required to provide the description of the linear program of the Dantzig–Wolfe decomposition of the master problem as well as the algorithm (possibly heuristic) solving the pricing problem. No branching rule implementation is required. If the pricing algorithm doesn't provide a bound, these primal algorithms still work, just without a dual bound.

If a bound for the pricing and a branching rule are provided, then it is possible to run an exact branch-and-price(-and-cut) instead.

Cutting-planes are also supported across algorithms, to strengthen the relaxation.

Solving a problem only requires a couple hundred lines of code (see examples).

A linear programming solver is required. Currently, CLP, Highs, Xpress and CPLEX are supported.

Features:
* Algorithms:
  * Column generation `column-generation`
  * Greedy `greedy`
  * Limited discrepancy search `limited-discrepancy-search`
  * Branch-and-price(-and-cut) `branch-and-price`
* Column generation options:
  * Internal diving
  * Rounding heuristic
  * Cutting planes
* Stabilization techniques:
  * Static and self-adjusting Wentges smoothing
  * Static and automatic directional smoothing

## Examples

Data can be downloaded from [fontanf/orproblems](https://github.com/fontanf/orproblems)

When the sub-problems can be solved with a very efficient algorithm - typically a pseudo-polynomial dynamic programming algorithm - then the bottleneck is the resolution of the linear problems. This is the case for the examples cutting stock, multiple knapsack, generalized assignment and star observation scheduling.

When the sub-problems are more difficult to solve, their resolution become the bottleneck of the algorithm. This is the case for the examples geometrical variable-sized bin packing, bin packing with conflicts, capacitated vehicle routing, vehicle routing problem with time windows and graph coloring. Here, these sub-problems are solved using generic approaches based on heuristic tree search or local search. During the first column generation iterations, these heuristic algorithms are stopped early to avoid spending a lot of time to find trivial columns.

### Packing

[Cutting stock problem](examples/cutting_stock_main.cpp)
* Pricing problem: bounded knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Multiple knapsack problem](examples/multiple_knapsack_main.cpp)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Generalized assignment problem](https://github.com/fontanf/generalizedassignmentsolver/blob/master/src/algorithms/column_generation.cpp) from [fontanf/generalizedassignmentsolver](https://github.com/fontanf/generalizedassignmentsolver)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Geometrical cutting stock, variable-sized bin packing and multiple knapsack problems](https://github.com/fontanf/packingsolver/blob/master/src/algorithms/column_generation.hpp) from [fontanf/packingsolver](https://github.com/fontanf/packingsolver)
* Pricing problem: geometrical knapsack problems solved with the algorithms from the same repository

[Bin packing problem with conflicts](examples/bin_packing_with_conflicts_main.cpp)
* Pricing problem: knapsack problem with conflicts solved with the [heuristic tree search](https://github.com/fontanf/treesearchsolver/blob/main/examples/knapsack_with_conflicts_main.cpp) algorithm from [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Routing

[Capacitated vehicle routing problem](examples/capacitated_vehicle_routing_main.cpp)
* Pricing problem: elementary shortest path problem with resource constraint [solved by heuristic tree search](examples/pricingsolver/espprc_main.cpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Vehicle routing problem with time windows](examples/vehicle_routing_with_time_windows_main.cpp)
* Pricing problem: elementary shortest path problem with resource constraint and time windows [solved by heuristic tree search](examples/pricingsolver/espprctw_main.cpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Scheduling

[Star observation scheduling problem](https://github.com/fontanf/starobservationschedulingsolver/blob/main/src/star_observation_scheduling/algorithms/column_generation.cpp) and [flexible star observation scheduling problem](https://github.com/fontanf/starobservationschedulingsolver/blob/main/src/flexible_star_observation_scheduling/algorithms/column_generation.cpp) from [fontanf/starobservationscheduling](https://github.com/fontanf/starobservationschedulingsolver)
* Pricing problem: single-night star observation scheduling problem [solved by dynamic programming](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/singlenightstarobservationscheduling/algorithms/dynamic_programming.hpp) and single-night flexible star observation scheduling problem [solved by dynamic programming](https://github.com/fontanf/starobservationschedulingsolver/blob/main/src/single_night_star_observation_scheduling/algorithms/dynamic_programming.cpp)

### Graphs

[Graph coloring problem](https://github.com/fontanf/coloringsolver/blob/master/src/algorithms/column_generation.cpp) from [fontanf/coloringsolver](https://github.com/fontanf/coloringsolver)
* Pricing problem: maximum-weight independent set problem solved with the `local-search` algorithm from [fontanf/stablesolver](https://github.com/fontanf/stablesolver) implemented with [fontanf/localsearchsolver](https://github.com/fontanf/localsearchsolver)

## Usage, running examples from command line

Compile:
```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
cmake --install build --config Release --prefix install
```

Download data:
```shell
python3 scripts/download_data.py
```

Examples:

```shell
./install/bin/columngenerationsolver_cutting_stock  --verbosity-level 1  --input "./data/cutting_stock/delorme2016/RG_CSP/BPP_1000_300_0.1_0.8_1.txt"  --format bpplib_csp  --algorithm greedy  --internal-diving 1  --rounding-heuristic 1
```
```
==========================================
          ColumnGenerationSolver          
==========================================

Model
-----
Objective sense:           Minimize
Number of constraints:     209
Number of static columns:  0

Algorithm
---------
Greedy

Parameters
----------
Time limit:                              inf
Messages
    Verbosity level:                     1
    Standard output:                     1
    File path:                           
    # streams:                           0
Logger
    Has logger:                          0
    Standard error:                      0
    File path:                           
Dummy column coef.:                      1
Number of columns in the column pool:    0
Number of initial columns:               0
Number of fixed columns:                 0
Number of branching decisions:           0
Number of initial cuts:                  0
Internal diving:                         1
Cutting planes:                          0
Rounding heuristic:                      1

Column generation
-----------------

        Time    Iter   # col       Value  Relaxation       Bound
        ----    ----   -----       -----  ----------       -----
       0.001       0     209         inf         209        -inf
       0.031       1     234         inf       191.3     -2808.7
       0.060       2     252         inf     191.197     -2808.7
       0.060       3     255         inf     191.143     -2808.7
       0.060       4     257         inf     191.143     -2808.7
       0.087       5     273         inf     191.117    -2415.03
[...]
       0.402      34     209         inf         836      191.06
       0.410      35     797         inf     446.727      191.06
       0.445      36     910         inf     425.284      191.06
       0.451      37     951         484     424.469      191.06
       0.454      38     966         484     424.408      191.06
[...]
       1.591     144    3604         446     443.633      423.96
       1.598     145    3624         446     443.613      423.96
       1.604     146    3635         444     443.589      423.96
       1.609     147    3645         444     443.585      423.96
[...]
       1.687     155    3715         444     443.493     443.493
       1.689     156    3717         444     443.493     443.493
       1.717     157    3725         444     443.493     443.493

Tree search
-----------

        Time       Value       Bound         Gap     Gap (%)                         Comment
        ----       -----       -----         ---     -------                         -------
       1.745         444     443.493    0.506734        0.11                          node 0
       1.749         444     443.493    0.506734        0.11                          node 1
[...]
       1.783         444     443.493    0.506734        0.11                         node 29
       1.783         444     443.493    0.506734        0.11                         node 30

Final statistics
----------------
Value:                         444
Bound:                         443.493
Absolute optimality gap:       0.506734
Relative optimality gap (%):   0.11426
Time:                          1.78348
Pricing time:                  1.22679
Linear programming time:       0.231017
Rounding heuristic time:       0.241049
Dummy column coef.:            16
Number of CG iterations:       320
Number of new columns:         3780
Number of nodes:               30

Solution
--------
Feasible:           1
Value:              444
Number of columns:  163
```

Note the `Value` column turning from `inf` to `484` at iteration 37: the rounding heuristic already found a feasible completion there, well before column generation converges (iteration 157) — that early solution is why `heuristic_tree_search`, which used to serve this role by branching after convergence, is no longer part of this package.

## Usage, C++ library

See examples.
