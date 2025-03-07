# ColumnGenerationSolver

A solver based on column generation.

![columngeneration](img/columngeneration.jpg?raw=true "columngeneration")

[image source](https://commons.wikimedia.org/wiki/File:ColonnesPavillonTrajan.jpg)

## Description

The goal of this package is to provide a simple framework to quickly implement heuristic algorithms based on column generation.

It is only required to provide the description of the linear program of the Dantzig–Wolfe decomposition of the master problem as well as the algorithm solving the pricing problem.
No branching rule implementation is required.

The currently implemented algorithms are based on the algorithms from "Primal Heuristics for Branch and Price: The Assets of Diving Methods" (Sadykov et al., 2019).

This package does not implement any exact algorithm. However, it provides a dual bound if the pricing algorithm provides provides a bound.
If the pricing algorithm doesn't provide a bound, the primal algorithms still works, but no dual bound is provided.

Solving a problem only requires a couple hundred lines of code (see examples).

A linear programming solver is required. Currently, CLP, Highs, Xpress and CPLEX are supported.

Features:
* Algorithms:
  * Column generation `column-generation`
  * Greedy `greedy`
  * Limited discrepancy search `limited-discrepancy-search`
  * Heuristic tree search `heuristic-tree-search`
* Sabilization technics:
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
./install/bin/columngenerationsolver_cutting_stock  --verbosity-level 1  --input "./data/cutting_stock/delorme2016/RG_CSP/BPP_1000_300_0.1_0.8_1.txt"  --format bpplib_csp  --algorithm greedy  --internal-diving 1
```
```
==========================================
          ColumnGenerationSolver          
==========================================

Model
-----
Objective sense:        Minimize
Number of constraints:  209
Number of columns:      0

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
Internal diving:                         1

Column generation
-----------------

        Time   Iteration   # columns                   Value                   Bound
        ----   ---------   ---------                   -----                   -----
       0.001           0         209                     209                    -inf
       0.012           1         234                   191.3                 -2808.7
       0.015           2         241                   191.3                 -2808.7
       0.022           3         259                 191.173                -2379.84
       0.022           4         261                 191.145                -2379.84
       0.022           5         262                 191.143                -2379.84
       0.025           6         272                 191.128                -2308.87
       0.028           7         280                 191.098                -1452.78
       0.033           8         292                 191.087                -1041.89
       0.046           9         307                 191.068                 154.178
       0.046          10         316                 191.064                 154.178
       0.046          11         319                 191.064                 154.178
       0.055          12         331                 191.061                 176.838
       0.055          13         338                  191.06                 176.838
       0.056          14         340                  191.06                 176.838
       0.056          15         342                  191.06                 176.838
       0.059          16         352                  191.06                 186.653
       0.063          17         362                  191.06                  191.06
       0.067          18         209                     836                  191.06
       0.069          19         398                 606.917                  191.06
       0.071          20         403                 595.684                  191.06
       0.074          21         411                 578.556                  191.06
       0.075          22         415                 570.729                  191.06
       0.078          23         425                 553.112                  191.06
       0.086          24         452                 501.425                  191.06
       0.092          25         469                 491.596                  191.06
       0.095          26         477                 481.099                  191.06
       0.099          27         485                 470.717                  191.06
       0.121          28         584                 432.913                  191.06
       0.122          29         587                 431.706                  191.06
       0.125          30         594                  427.53                  191.06
       0.132          31         613                 425.594                  191.06
       0.144          32         649                 424.966                 316.056
       0.170          33         718                 424.253                 382.123
       0.172          34         742                 424.115                 382.123
       0.173          35         744                 424.115                 382.123
       0.178          36         753                 424.036                 411.192
       0.178          37         757                 424.033                 411.192
       0.182          38         764                 424.014                 411.192
       0.182          39         765                 424.013                 411.192
       0.195          40         811                 423.963                 417.335
       0.198          41         816                 423.963                  420.63
       0.202          42         828                  423.96                  420.63
       0.205          43         838                  423.96                  423.96
       0.210          44         209                    3344                  423.96
       0.223          45         854                  628.09                  423.96
       0.230          46         864                 581.435                  423.96
       0.238          47         878                 524.335                  423.96
       0.243          48         886                 512.584                  423.96
       0.270          49         953                 450.541                  423.96
       0.272          50         967                 445.064                  423.96
       0.273          51         969                 445.062                  423.96
       0.283          52         998                 443.553                  423.96
       0.306          53        1037                 443.499                 435.159
       0.307          54        1043                 443.494                 435.159
       0.307          55        1044                 443.494                 435.159
       0.327          56        1077                 443.493                 441.402
       0.327          57        1079                 443.493                 441.402
       0.347          58        1115                 443.493                 443.493

Tree search
-----------

        Time       Value       Bound         Gap     Gap (%)                         Comment
        ----       -----       -----         ---     -------                         -------
       0.366         inf     443.493         inf         inf                          node 0
       0.369         inf     443.493         inf         inf                          node 1
       0.371         inf     443.493         inf         inf                          node 2
       0.372         inf     443.493         inf         inf                          node 3
       0.373         inf     443.493         inf         inf                          node 4
       0.374         inf     443.493         inf         inf                          node 5
       0.375         inf     443.493         inf         inf                          node 6
       0.376         inf     443.493         inf         inf                          node 7
       0.377         inf     443.493         inf         inf                          node 8
       0.379         inf     443.493         inf         inf                          node 9
       0.379         inf     443.493         inf         inf                         node 10
       0.380         inf     443.493         inf         inf                         node 11
       0.381         inf     443.493         inf         inf                         node 12
       0.382         inf     443.493         inf         inf                         node 13
       0.384         inf     443.493         inf         inf                         node 14
       0.385         inf     443.493         inf         inf                         node 15
       0.386         inf     443.493         inf         inf                         node 16
       0.387         inf     443.493         inf         inf                         node 17
       0.390         inf     443.493         inf         inf                         node 18
       0.391         inf     443.493         inf         inf                         node 19
       0.393         inf     443.493         inf         inf                         node 20
       0.394         inf     443.493         inf         inf                         node 21
       0.395         inf     443.493         inf         inf                         node 22
       0.395         inf     443.493         inf         inf                         node 23
       0.397         inf     443.493         inf         inf                         node 24
       0.398         inf     443.493         inf         inf                         node 25
       0.398         inf     443.493         inf         inf                         node 26
       0.399         inf     443.493         inf         inf                         node 27
       0.399         inf     443.493         inf         inf                         node 28
       0.400         inf     443.493         inf         inf                         node 29
       0.400         inf     443.493         inf         inf                         node 30
       0.400         inf     443.493         inf         inf                         node 31
       0.400         444     443.493    0.506734        0.11                         node 32

Final statistics
----------------
Value:                         444
Bound:                         443.493
Absolute optimality gap:       0.506734
Relative optimality gap (%):   0.11426
Time:                          0.400473
Pricing time:                  0.290814
Linear programming time:       0.0922988
Dummy column coef.:            16
Number of CG iterations:       258
Number of new columns:         1057
Number of nodes:               32

Solution
--------
Feasible:           1
Value:              444
Number of columns:  184
```

## Usage, C++ library

See examples.
