Column Generation Solver
===

A solver based on column generation.

![columngeneration](img/columngeneration.jpg?raw=true "columngeneration")

[image source](https://commons.wikimedia.org/wiki/File:ColonnesPavillonTrajan.jpg)

# Description

The goal of this repository is to provide a simple framework to quickly implement heuristic algorithms based on column generation.

It is only required to provide the description of the linear program of the Dantzig–Wolfe decomposition of the master problem as well as the algorithm solving the pricing problem.
No branching rule implementation is required.

The currently implemented algorithms are based on the algorithms from "Primal Heuristics for Branch and Price: The Assets of Diving Methods" (Sadykov et al., 2019).

This package does not implement any exact algorithm. However, if the pricing algorithm is exact, it provides a valid dual bound.
If the pricing algorithm is heuristic, the primal algorithms still works, but then the dual bound is not valid.

This is a header-only library. Implementing a problem only requires a few hundred lines of code (see examples).

A linear programming solver is required. Currently, CLP and CPLEX are supported.

Features:
* Algorithms:
  * Column generation `columngeneration`
  * Greedy `greedy`
  * Limited Discrepancy Search `limiteddiscrepancysearch`
* Sabilization technics:
  * Static and self-adjusting Wentges smoothing
  * Static and automatic directional smoothing

# Examples

Cutting Stock Problem
* Pricing problem: Bounded Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

Multiple Knapsack Problem
* Pricing problem: Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

Generalized Assignment Problem from [fontanf/generalizedassignmentsolver](https://github.com/fontanf/generalizedassignmentsolver/blob/master/generalizedassignmentsolver/algorithms/columngeneration.cpp)
* Pricing problem: Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

<!--Geometrical Cutting Stock and Variable-sized Bin Packing Problems from [fontanf/packingsolver](https://github.com/fontanf/packingsolver)-->
<!--* Pricing problem: Geometrical Knapsack Problems solved with the algorithms from the same repository-->

Graph Coloring Problem from [fontanf/coloringsolver](https://github.com/fontanf/coloringsolver/blob/master/coloringsolver/algorithms/columngeneration.cpp)
* Pricing problem: Maximum-Weight Independent Set Problem solved with the `largeneighborhoodsearch` algorithm from [fontanf/stablesolver](https://github.com/fontanf/stablesolver)

# Usage, running examples from command line

You need to have a linear programming solver already installed. Then update the corresponding entry in the `WORKSPACE` file. You may only need to update the `path` attribute of the solver you are using. Then, compile with one of the following command:
```shell
bazel build --define coinor=true -- //...
bazel build --define cplex=true -- //...
```

Then, examples can be executed as follows:
```shell
./bazel-bin/examples/main -v -p cuttingstock -a "columngeneration" -i "data/cuttingstock/falkenauer1996/T/Falkenauer_t120_00.txt"
./bazel-bin/examples/main -v -p multipleknapsack -a "limiteddiscrepancysearch" -i "data/multipleknapsack/FK_1/random10_100_4_1000_1_1.txt"
```

# Usage, C library

See examples.

