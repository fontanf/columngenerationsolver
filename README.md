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

[Cutting stock problem](examples/cuttingstock.hpp)
* Pricing problem: bounded knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Multiple knapsack problem](examples/multipleknapsack.hpp)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Generalized assignment problem](https://github.com/fontanf/generalizedassignmentsolver/blob/master/generalizedassignmentsolver/algorithms/columngeneration.cpp) from [fontanf/generalizedassignmentsolver](https://github.com/fontanf/generalizedassignmentsolver)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Geometrical cutting stock, variable-sized bin packing and multiple knapsack problems](https://github.com/fontanf/packingsolver/blob/master/packingsolver/algorithms/column_generation.hpp) from [fontanf/packingsolver](https://github.com/fontanf/packingsolver)
* Pricing problem: geometrical knapsack problems solved with the algorithms from the same repository

[Bin packing problem with conflicts](examples/binpackingwithconflicts.hpp)
* Pricing problem: knapsack problem with conflicts solved with the [heuristic tree search](https://github.com/fontanf/treesearchsolver/blob/main/examples/knapsackwithconflicts.hpp) algorithm from [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Routing

[Capacitated vehicle routing problem](examples/capacitatedvehiclerouting.hpp)
* Pricing problem: elementary shortest path problem with resource constraint [solved by heuristic tree search](examples/pricingsolver/espprc.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Vehicle routing problem with time windows](examples/vehicleroutingwithtimewindows.hpp)
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

Examples:

```shell
./bazel-bin/examples/cuttingstock_main  --verbosity-level 1  --input "../ordata/cuttingstock/delorme2016/RG/BPP_1000_300_0.1_0.8_1.txt"  --algorithm column-generation  --internal-diving 1
```
```
==========================================
          ColumnGenerationSolver          
==========================================

Model
-----
Number of constraints:               1000
Column lower bound:                  0
Column upper bound:                  1
Dummy column objective coefficient:  2

Algorithm
---------
Column generation

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
Number of columns in the column pool:    0
Number of initial columns:               0
Internal diving:                         1
Linear programming solver:               CLP
Static Wentges smoothing parameter:      0
Static directional smoothing parameter:  0
Self-adjusting Wentges smoothing:        0
Automatic directional smoothing:         0
Maximum number of iterations:            -1

Column generation
-----------------

        Time   Iteration   # columns                   Value
        ----   ---------   ---------                   -----
       0.006           1        1000                    2000
       0.163           2        1618                     618
       0.287           3        1739                     618
       0.411           4        1859                     618
       0.521           5        2055                     559
       0.654           6        2221                 545.333
       0.826           7        2350                 529.083
       1.012           8        2524                 519.362
       1.335           9        2726                 497.642
       1.673          10        2936                 487.687
       2.083          11        3138                 470.041
       2.500          12        3324                 463.345
       2.933          13        3534                 454.035
       3.401          14        3688                 450.498
       3.835          15        3869                 447.984
       4.449          16        4074                 446.407
       5.038          17        4252                 445.558
       5.621          18        4419                  444.91
       6.336          19        4575                 444.423
       7.128          20        4725                 444.142
       7.922          21        4875                 443.927
       8.736          22        5008                 443.789
       9.618          23        5146                 443.665
      10.521          24        5278                 443.597
      11.749          25        5386                  443.55
      12.949          26        5488                  443.52
      14.321          27        5505                 443.507
      15.779          28        5512                 443.503
      17.440          29        5517                 443.497
      18.963          30        5522                 443.494
      20.443          31        5612                 443.493
      21.933          32        5613                 443.493

Final statistics
----------------
Value:                         inf
Bound:                         443.493
Absolute optimality gap:       inf
Relative optimality gap (%):   inf
Time:                          23.4118
Pricing time:                  22.0377
LP time:                       1.31781
# of CG iterations:            32
Number of pricings:            32
Number of first-try pricings:  32
Number of mispricings:         0
Number of no-stab pricings:    32

Solution
--------
Feasible:           0
Value:              0
Number of columns:  0
```

```shell
./bazel-bin/examples/multipleknapsack_main  --verbosity-level 1  --input "../ordata/multipleknapsack/fukunaga2011/FK_1/random10_100_4_1000_1_1.txt"  --algorithm greedy
```
```
==========================================
          ColumnGenerationSolver          
==========================================

Model
-----
Number of constraints:               110
Column lower bound:                  0
Column upper bound:                  1
Dummy column objective coefficient:  -510

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
Number of columns in the column pool:    0
Number of initial columns:               0
Internal diving:                         0

Column generation
-----------------

        Time   Iteration   # columns                   Value
        ----   ---------   ---------                   -----
       0.001           1          10                   -5100
       0.002           2          20                     489
       0.003           3          30                 4303.14
       0.004           4          40                    7049
       0.006           5          50                 10619.7
       0.007           6          60                 14990.4
       0.009           7          70                 17049.8
       0.011           8          80                 22944.4
       0.012           9          90                 25914.4
       0.014          10         100                   26797

Tree search
-----------

        Time       Value       Bound         Gap     Gap (%)                         Comment
        ----       -----       -----         ---     -------                         -------
       0.016        -inf       26797         inf         inf                          node 0
       0.021        -inf       26797         inf         inf                          node 1
       0.023        -inf       26797         inf         inf                          node 2
       0.025        -inf       26797         inf         inf                          node 3
       0.028        -inf       26797         inf         inf                          node 4
       0.029        -inf       26797         inf         inf                          node 5
       0.031        -inf       26797         inf         inf                          node 6
       0.032        -inf       26797         inf         inf                          node 7
       0.032        -inf       26797         inf         inf                          node 8
       0.033       26797       26797 2.88856e-09        0.00                          node 9

Final statistics
----------------
Value:                         26797
Bound:                         26797
Absolute optimality gap:       2.88856e-09
Relative optimality gap (%):   1.07794e-11
Time:                          0.0328832
Pricing time:                  0.0233441
LP time:                       0.00692869
# of CG iterations:            40
Number of nodes:               10

Solution
--------
Feasible:           1
Value:              26797
Number of columns:  10
```

## Usage, C++ library

See examples.
