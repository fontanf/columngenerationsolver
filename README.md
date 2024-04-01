# ColumnGenerationSolver

A solver based on column generation.

![columngeneration](img/columngeneration.jpg?raw=true "columngeneration")

[image source](https://commons.wikimedia.org/wiki/File:ColonnesPavillonTrajan.jpg)

## Description

The goal of this repository is to provide a simple framework to quickly implement heuristic algorithms based on column generation.

It is only required to provide the description of the linear program of the Dantzig–Wolfe decomposition of the master problem as well as the algorithm solving the pricing problem.
No branching rule implementation is required.

The currently implemented algorithms are based on the algorithms from "Primal Heuristics for Branch and Price: The Assets of Diving Methods" (Sadykov et al., 2019).

This package does not implement any exact algorithm. However, if the pricing algorithm is exact, it provides a valid dual bound.
If the pricing algorithm is heuristic, the primal algorithms still works, but then the dual bound is not valid.

Solving a problem only requires a couple hundred lines of code (see examples).

A linear programming solver is required. Currently, CLP, Xpress and CPLEX are supported.

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

[Cutting stock problem](examples/cutting_stock.hpp)
* Pricing problem: bounded knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Multiple knapsack problem](examples/multiple_knapsack.hpp)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Generalized assignment problem](https://github.com/fontanf/generalizedassignmentsolver/blob/master/generalizedassignmentsolver/algorithms/columngeneration.cpp) from [fontanf/generalizedassignmentsolver](https://github.com/fontanf/generalizedassignmentsolver)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Geometrical cutting stock, variable-sized bin packing and multiple knapsack problems](https://github.com/fontanf/packingsolver/blob/master/packingsolver/algorithms/column_generation.hpp) from [fontanf/packingsolver](https://github.com/fontanf/packingsolver)
* Pricing problem: geometrical knapsack problems solved with the algorithms from the same repository

[Bin packing problem with conflicts](examples/bin_packing_with_conflicts.hpp)
* Pricing problem: knapsack problem with conflicts solved with the [heuristic tree search](https://github.com/fontanf/treesearchsolver/blob/main/examples/knapsack_with_conflicts.hpp) algorithm from [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Routing

[Capacitated vehicle routing problem](examples/capacitated_vehicle_routing.hpp)
* Pricing problem: elementary shortest path problem with resource constraint [solved by heuristic tree search](examples/pricingsolver/espprc.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Vehicle routing problem with time windows](examples/vehicle_routing_with_time_windows.hpp)
* Pricing problem: elementary shortest path problem with resource constraint and time windows [solved by heuristic tree search](examples/pricingsolver/espprctw.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Scheduling

[Star observation scheduling problem](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/starobservationscheduling/algorithms/column_generation.cpp) and [flexible star observation scheduling problem](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/flexiblestarobservationscheduling/algorithms/column_generation.cpp) from [fontanf/starobservationscheduling](https://github.com/fontanf/starobservationschedulingsolver)
* Pricing problem: single-night star observation scheduling problem [solved by dynamic programming](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/singlenightstarobservationscheduling/algorithms/dynamic_programming.hpp) and single-night flexible star observation scheduling problem [solved by dynamic programming](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/flexiblesinglenightstarobservationscheduling/algorithms/dynamic_programming.hpp)

### Graphs

[Graph coloring problem](https://github.com/fontanf/coloringsolver/blob/master/coloringsolver/algorithms/column_generation.cpp) from [fontanf/coloringsolver](https://github.com/fontanf/coloringsolver)
* Pricing problem: maximum-weight independent set problem solved with the `local-search` algorithm from [fontanf/stablesolver](https://github.com/fontanf/stablesolver) implemented with [fontanf/localsearchsolver](https://github.com/fontanf/localsearchsolver)

## Usage, running examples from command line

You need to have a linear programming solver already installed. Then update the corresponding entry in the `WORKSPACE` file. You may only need to update the `path` attribute of the solver you are using. Then, compile with one of the following command:
```shell
bazel build --define clp=true -- //...
bazel build --define cplex=true -- //...
```

Download data:
```shell
python3 scripts/download_data.py
```

Examples:

```shell
./bazel-bin/examples/cutting_stock_main  --verbosity-level 1  --input "./data/cutting_stock/delorme2016/RG_CSP/BPP_1000_300_0.1_0.8_1.txt"  --format bpplib_csp  --algorithm greedy  --internal-diving 1
```
```
==========================================
          ColumnGenerationSolver          
==========================================

Model
-----
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

        Time   Iteration   # columns                   Value
        ----   ---------   ---------                   -----
       0.001           0         209                     209
       0.012           1         234                   191.3
       0.015           2         241                   191.3
       0.022           3         259                 191.173
       0.023           4         261                 191.145
       0.023           5         262                 191.143
       0.027           6         272                 191.128
       0.030           7         280                 191.098
       0.035           8         292                 191.087
       0.048           9         307                 191.068
       0.048          10         316                 191.064
       0.049          11         319                 191.064
       0.058          12         331                 191.061
       0.059          13         338                  191.06
       0.060          14         340                  191.06
       0.060          15         342                  191.06
       0.064          16         352                  191.06
       0.068          17         362                  191.06
       0.073          18         209                     836
       0.075          19         398                 606.917
       0.077          20         403                 595.684
       0.081          21         411                 578.556
       0.083          22         415                 570.729
       0.087          23         425                 553.112
       0.095          24         452                 501.425
       0.101          25         469                 491.596
       0.105          26         477                 481.099
       0.109          27         485                 470.717
       0.132          28         584                 432.913
       0.134          29         587                 431.706
       0.137          30         594                  427.53
       0.145          31         613                 425.594
       0.157          32         649                 424.966
       0.184          33         718                 424.253
       0.186          34         742                 424.115
       0.187          35         744                 424.115
       0.193          36         753                 424.036
       0.194          37         757                 424.033
       0.198          38         764                 424.014
       0.200          39         765                 424.013
       0.214          40         811                 423.963
       0.216          41         816                 423.963
       0.221          42         828                  423.96
       0.225          43         838                  423.96
       0.230          44         209                    3344
       0.243          45         854                  628.09
       0.252          46         864                 581.435
       0.261          47         878                 524.335
       0.266          48         886                 512.584
       0.294          49         953                 450.541
       0.297          50         967                 445.064
       0.298          51         969                 445.062
       0.309          52         998                 443.553
       0.333          53        1037                 443.499
       0.335          54        1043                 443.494
       0.337          55        1044                 443.494
       0.357          56        1077                 443.493
       0.358          57        1079                 443.493
       0.378          58        1115                 443.493

Tree search
-----------

        Time       Value       Bound         Gap     Gap (%)                         Comment
        ----       -----       -----         ---     -------                         -------
       0.397         inf     443.493         inf         inf                          node 0
       0.402         inf     443.493         inf         inf                          node 1
       0.405         inf     443.493         inf         inf                          node 2
       0.407         inf     443.493         inf         inf                          node 3
       0.409         inf     443.493         inf         inf                          node 4
       0.411         inf     443.493         inf         inf                          node 5
       0.413         inf     443.493         inf         inf                          node 6
       0.415         inf     443.493         inf         inf                          node 7
       0.417         inf     443.493         inf         inf                          node 8
       0.418         inf     443.493         inf         inf                          node 9
       0.420         inf     443.493         inf         inf                         node 10
       0.422         inf     443.493         inf         inf                         node 11
       0.424         inf     443.493         inf         inf                         node 12
       0.428         inf     443.493         inf         inf                         node 13
       0.430         inf     443.493         inf         inf                         node 14
       0.432         inf     443.493         inf         inf                         node 15
       0.434         inf     443.493         inf         inf                         node 16
       0.435         inf     443.493         inf         inf                         node 17
       0.439         inf     443.493         inf         inf                         node 18
       0.440         inf     443.493         inf         inf                         node 19
       0.442         inf     443.493         inf         inf                         node 20
       0.443         inf     443.493         inf         inf                         node 21
       0.443         inf     443.493         inf         inf                         node 22
       0.445         inf     443.493         inf         inf                         node 23
       0.446         inf     443.493         inf         inf                         node 24
       0.448         inf     443.493         inf         inf                         node 25
       0.450         inf     443.493         inf         inf                         node 26
       0.452         inf     443.493         inf         inf                         node 27
       0.452         inf     443.493         inf         inf                         node 28
       0.453         inf     443.493         inf         inf                         node 29
       0.454         inf     443.493         inf         inf                         node 30
       0.454         444     443.493    0.506734        0.11                         node 31

Final statistics
----------------
Value:                         444
Bound:                         443.493
Absolute optimality gap:       0.506734
Relative optimality gap (%):   0.11426
Time:                          0.453859
Pricing time:                  0.287987
Linear programming time:       0.149853
Dummy column coef.:            16
Number of CG iterations:       245
Number of new columns:         1040
Number of nodes:               31

Solution
--------
Feasible:           1
Value:              444
Number of columns:  187
```

## Usage, C++ library

See examples.
