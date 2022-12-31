# Column Generation Solver

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
  * Column generation `column_generation`
  * Greedy `greedy`
  * Limited Discrepancy Search `limited_discrepancy_search`
  * Heuristic Tree Search `heuristic_tree_search`
* Sabilization technics:
  * Static and self-adjusting Wentges smoothing
  * Static and automatic directional smoothing

## Examples

Data can be downloaded from [fontanf/orproblems](https://github.com/fontanf/orproblems)

When the sub-problems can be solved with a very efficient algorithm - typically a pseudo-polynomial dynamic programming algorithm - then the bottleneck is the resolution of the linear problems. This is the case for the examples Cutting Stock, Multiple Knapsack, Generalized Assignment and Star Observation Scheduling.

When the sub-problems are more difficult to solve, their resolution become the bottleneck of the algorithm. This is the case for the examples Geometrical Variable-sized Bin Packing, Bin Packing with Conflicts, Capacitated Vehicle Routing, Vehicle Routing Problem with Time Windows and Graph Coloring. Here, these sub-problems are solved using generic approaches based on Heuristic Tree Search or Local Search. During the first column generation iterations, these heuristic algorithms are stopped early to avoid spending a lot of time to find trivial columns.

### Packing

[Cutting Stock Problem](examples/cuttingstock.hpp)
* Pricing problem: Bounded Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

<details><summary>Benchmarks</summary>
<p>

* Benchmarks:
  * `python3 ../optimizationtools/optimizationtools/bench_run.py --main ./bazel-bin/examples/cuttingstock_main --csv ../ordata/cuttingstock/data.csv -l cuttingstock -a "heuristic_tree_search" -t 60`

</p>
</details>

[Multiple Knapsack Problem](examples/multipleknapsack.hpp)
* Pricing problem: Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

<details><summary>Benchmarks</summary>
<p>

* Benchmarks:
  * `python3 ../optimizationtools/optimizationtools/bench_run.py --main ./bazel-bin/examples/multipleknapsack_main --csv ../ordata/multipleknapsack/data.csv -l multipleknapsack -a "heuristic_tree_search" -t 10`

</p>
</details>

Generalized Assignment Problem from [fontanf/generalizedassignmentsolver](https://github.com/fontanf/generalizedassignmentsolver/blob/master/generalizedassignmentsolver/algorithms/columngeneration.cpp)
* Pricing problem: Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

Geometrical Cutting Stock and Variable-sized Bin Packing Problems from [fontanf/packingsolver](https://github.com/fontanf/packingsolver/blob/master/packingsolver/algorithms/column_generation.hpp)
* Pricing problem: Geometrical Knapsack Problems solved with the algorithms from the same repository

[Bin Packing Problem with Conflicts](examples/binpackingwithconflicts.hpp)
* Pricing problem: Knapsack Problem with Conflicts solved with the [Heuristic Tree Search](https://github.com/fontanf/treesearchsolver/blob/main/examples/knapsackwithconflicts.hpp) algorithm from [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Routing

[Capacitated Vehicle Routing Problem](examples/capacitatedvehiclerouting.hpp)
* Pricing problem: Elementary Shortest Path Problem with Resource Constraint [solved by Heuristic Tree Search](examples/pricingsolver/espprc.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Vehicle Routing Problem with Time Windows](examples/vehicleroutingwithtimewindows.hpp)
* Pricing problem: Elementary Shortest Path Problem with Resource Constraint and Time Windows [solved by Heuristic Tree Search](examples/pricingsolver/espprctw.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

<details><summary>Benchmarks</summary>
<p>

```shell
DATE=$(date '+%Y-%m-%d--%H-%M-%S') && python3 ../optimizationtools/scripts/bench_run.py --main ./bazel-bin/examples/vehicleroutingwithtimewindows_main --csv ../ordata/vehicleroutingwithtimewindows/data.csv -f "row['Dataset'] == 'solomon1987'" -l "${DATE}_vehicleroutingwithtimewindows"" -a "limited_discrepancy_search" -t 120
python3 ../optimizationtools/scripts/bench_process.py --csv ../ordata/vehicleroutingwithtimewindows/data.csv -f "row['Dataset'] == 'solomon1987'" -l "${DATE}_vehicleroutingwithtimewindows" -b heuristiclong -t 62
```

</p>
</details>

### Scheduling

[Star Observation Scheduling Problem](examples/starobservationscheduling.hpp)
* Pricing problem: Single Night Star Observation Scheduling Problem [solved by dynamic programming](examples/pricingsolver/singlenightstarobservationscheduling.hpp)

<details><summary>Benchmarks</summary>
<p>

* Benchmarks:
  * `python3 ../optimizationtools/optimizationtools/bench_run.py --main ./bazel-bin/examples/starobservationscheduling_main --csv ../ordata/starobservationscheduling/data.csv -l starobservationscheduling -a "heuristic_tree_search" -t 60`

</p>
</details>

### Graphs

Graph Coloring Problem from [fontanf/coloringsolver](https://github.com/fontanf/coloringsolver/blob/master/coloringsolver/algorithms/columngeneration.cpp)
* Pricing problem: Maximum-Weight Independent Set Problem solved with the `localsearch` algorithm from [fontanf/stablesolver](https://github.com/fontanf/stablesolver) implemented with [fontanf/localsearchsolver](https://github.com/fontanf/localsearchsolver)

## Usage, running examples from command line

You need to have a linear programming solver already installed. Then update the corresponding entry in the `WORKSPACE` file. You may only need to update the `path` attribute of the solver you are using. Then, compile with one of the following command:
```shell
bazel build --define clp=true -- //...
bazel build --define cplex=true -- //...
```

Then, examples can be executed as follows:
```shell
./bazel-bin/examples/cuttingstock_main -v 1 -a "column_generation" -i "../ordata/cuttingstock/falkenauer1996/T/Falkenauer_t120_00.txt"
./bazel-bin/examples/multipleknapsack_main -v 1 -a "limited_discrepancy_search" -i "../ordata/multipleknapsack/fukunaga2011/FK_1/random10_100_4_1000_1_1.txt"
```

## Usage, C++ library

See examples.

