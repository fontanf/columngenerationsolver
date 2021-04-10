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

A linear programming solver is required. Currently, CLP and CPLEX are supported.

Features:
* Algorithms:
  * Column generation `columngeneration`
  * Greedy `greedy`
  * Limited Discrepancy Search `limiteddiscrepancysearch`
* Sabilization technics:
  * Static and self-adjusting Wentges smoothing
  * Static and automatic directional smoothing

## Examples

### Packing

[Cutting Stock Problem](examples/cuttingstock.hpp)
* Pricing problem: Bounded Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Multiple Knapsack Problem](examples/multipleknapsack.hpp)
* Pricing problem: Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

Generalized Assignment Problem from [fontanf/generalizedassignmentsolver](https://github.com/fontanf/generalizedassignmentsolver/blob/master/generalizedassignmentsolver/algorithms/columngeneration.cpp)
* Pricing problem: Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

Geometrical Cutting Stock and Variable-sized Bin Packing Problems from [fontanf/packingsolver](https://github.com/fontanf/packingsolver/blob/master/packingsolver/algorithms/column_generation.hpp)
* Pricing problem: Geometrical Knapsack Problems solved with the algorithms from the same repository

[Bin Packing Problem with Conflicts](examples/binpackingwithconflicts.hpp)
* Literature:
  * "Algorithms for the Bin Packing Problem with Conflicts" (Muritiba et al., 2010) [DOI](https://doi.org/10.1287/ijoc.1090.0355)
  * "A Branch-and-Price Algorithm for the Bin Packing Problem with Conflicts" (Elhedhli et al., 2010) [DOI](https://doi.org/10.1287/ijoc.1100.0406)
  * "Bin Packing with Conflicts: A Generic Branch-and-Price Algorithm" (Sadykov et Vanderbeck, 2012) [DOI](https://doi.org/10.1287/ijoc.1120.0499)
  * "Bin packing and related problems: General arc-flow formulation with graph compression" (Brandão et Pedroso, 2016) [DOI](https://doi.org/10.1016/j.cor.2015.11.009)
  * "A New Branch-and-Price-and-Cut Algorithm for One-Dimensional Bin-Packing Problems" (Wei et al., 2019) [DOI](https://doi.org/10.1287/ijoc.2018.0867)
* Pricing problem: Knapsack Problem with Conflicts solved with the [Heuristic Tree Search](https://github.com/fontanf/treesearchsolver/blob/main/examples/knapsackwithconflicts.hpp) algorithm from [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Routing

[Capacitated Vehicle Routing Problem](examples/capacitatedvehiclerouting.hpp)
* Pricing problem: Elementary Shortest Path Problem with Resource Constraint [solved by Heuristic Tree Search](examples/pricingsolver/espprc.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Vehicle Routing Problem with Time Windows](examples/vehicleroutingwithtimewindows.hpp)
* Time windows / Capacity constraint / Maximum number of vehicles
* Pricing problem: Elementary Shortest Path Problem with Resource Constraint and Time Windows [solved by Heuristic Tree Search](examples/pricingsolver/espprctw.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Capacitated Open Vehicle Routing Problem](examples/capacitatedopenvehiclerouting.hpp)
* No need to return to the depot / Capacity constraint / Maximum route length / Maximum number of vehicles
* Literature:
  * "A hybrid evolution strategy for the open vehicle routing problem" (Repoussis et al., 2010) [DOI](https://doi.org/10.1016/j.cor.2008.11.003)
* Pricing problem: Elementary Open Shortest Path Problem with Resource Constraints [solved by Heuristic Tree Search](examples/pricingsolver/eospprc.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Scheduling

[Identical parallel machine scheduling problem with family setup times, Total weighted completion time](examples/parallelschedulingwithfamilysetuptimestwct.hpp)
* Three field classification: `P | sᵢ | ∑wⱼCⱼ`
* Literature:
  * "Heuristic methods for the identical parallel machine flowtime problem with set-up times" (Dunstall et Wirth, 2005) [DOI](https://doi.org/10.1016/j.cor.2004.03.013)
  * "An improved heuristic for parallel machine weighted flowtime scheduling with family set-up times" (Liao et al., 2012) [DOI](https://doi.org/10.1016/j.camwa.2011.10.077)
  * "Mathematical formulations for scheduling jobs on identical parallel machines with family setup times and total weighted completion time minimization" (Kramer et al., 2021) [DOI](https://doi.org/10.1016/j.ejor.2019.07.006)
* Pricing problem: Single machine order acceptance and scheduling problem with family setup times, Total weighted completion time [solved by Heuristic Tree Search](examples/pricingsolver/oaschedulingwithfamilysetuptimestwct.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Star Observation Scheduling Problem](examples/starobservationscheduling.hpp)
* Three field classification: `R | rⱼᵢ, 2 pⱼᵢ ≥ dⱼᵢ - rⱼᵢ | ∑wⱼUⱼ`
* Literature:
  * "A Branch-And-Price Algorithm for Scheduling Observations on a Telescope" (Catusse et al., 2016)
* Pricing problem: Single Night Star Observation Scheduling Problem [solved by dynamic programming](examples/pricingsolver/singlenightstarobservationscheduling.hpp)

### Graphs

Graph Coloring Problem from [fontanf/coloringsolver](https://github.com/fontanf/coloringsolver/blob/master/coloringsolver/algorithms/columngeneration.cpp)
* Pricing problem: Maximum-Weight Independent Set Problem solved with the `largeneighborhoodsearch` algorithm from [fontanf/stablesolver](https://github.com/fontanf/stablesolver)

## Usage, running examples from command line

[Download data](https://github.com/fontanf/columngenerationsolver/releases/download/data/columngenerationdata.zip)

You need to have a linear programming solver already installed. Then update the corresponding entry in the `WORKSPACE` file. You may only need to update the `path` attribute of the solver you are using. Then, compile with one of the following command:
```shell
bazel build --define coinor=true -- //...
bazel build --define cplex=true -- //...
```

Then, examples can be executed as follows:
```shell
./bazel-bin/examples/main -v -p cuttingstock -a "columngeneration" -i "data/cuttingstock/falkenauer1996/T/Falkenauer_t120_00.txt"
./bazel-bin/examples/main -v -p multipleknapsack -a "limiteddiscrepancysearch" -i "data/multipleknapsack/fukunaga2011/FK_1/random10_100_4_1000_1_1.txt"
```

## Usage, C++ library

See examples.

## Benchmarks

```shell
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/cuttingstock/data.csv -l cuttingstock_columngeneration --main "./bazel-bin/examples/main -p cuttingstock -a columngeneration" -g \"--linear-programming-solver CPLEX\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/cuttingstock/data.csv -l cuttingstock_columngeneration_wentges --main "./bazel-bin/examples/main -p cuttingstock -a columngeneration -g \"--linear-programming-solver CPLEX --self-adjusting-wentges-smoothing 1\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/cuttingstock/data.csv -l cuttingstock_columngeneration_wentges_directional --main "./bazel-bin/examples/main -p cuttingstock -a columngeneration -g \"--linear-programming-solver CPLEX --self-adjusting-wentges-smoothing 1 --automatic-directional-smoothing 1\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/cuttingstock/data.csv -l cuttingstock_greedy_wentges_directional --main "./bazel-bin/examples/main -p cuttingstock -a greedy -g \"--linear-programming-solver CPLEX --self-adjusting-wentges-smoothing 1 --automatic-directional-smoothing 1\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/multipleknapsack/data.csv -l multipleknapsack_columngeneration --main "./bazel-bin/examples/main -p multipleknapsack -a columngeneration" -g \"--linear-programming-solver CPLEX\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/multipleknapsack/data.csv -l multipleknapsack_columngeneration_wentges --main "./bazel-bin/examples/main -p multipleknapsack -a columngeneration -g \"--linear-programming-solver CPLEX --self-adjusting-wentges-smoothing 1\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/multipleknapsack/data.csv -l multipleknapsack_columngeneration_wentges_directional --main "./bazel-bin/examples/main -p multipleknapsack -a columngeneration -g \"--linear-programming-solver CPLEX --self-adjusting-wentges-smoothing 1 --automatic-directional-smoothing 1\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/multipleknapsack/data.csv -l multipleknapsack_greedy_wentges_directional --main "./bazel-bin/examples/main -p multipleknapsack -a greedy -g \"--linear-programming-solver CPLEX --self-adjusting-wentges-smoothing 1 --automatic-directional-smoothing 1\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/starobservationscheduling/data.csv -l starobservationscheduling_columngeneration --main "./bazel-bin/examples/main -p starobservationscheduling -a columngeneration" -g \"--linear-programming-solver CPLEX\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/starobservationscheduling/data.csv -l starobservationscheduling_columngeneration_wentges --main "./bazel-bin/examples/main -p starobservationscheduling -a columngeneration -g \"--linear-programming-solver CPLEX --self-adjusting-wentges-smoothing 1\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/starobservationscheduling/data.csv -l starobservationscheduling_columngeneration_wentges_directional --main "./bazel-bin/examples/main -p starobservationscheduling -a columngeneration -g \"--linear-programming-solver CPLEX --self-adjusting-wentges-smoothing 1 --automatic-directional-smoothing 1\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/starobservationscheduling/data.csv -l starobservationscheduling_greedy_cplex --main "./bazel-bin/examples/main -p starobservationscheduling -a greedy -g \"--linear-programming-solver CPLEX\""
python3 ../optimizationtools/optimizationtools/bench_run.py --csv data/parallelschedulingwithfamilysetuptimestwct/data.csv -l parallelschedulingwithfamilysetuptimestwct --main "./bazel-bin/examples/main -p parallelschedulingwithfamilysetuptimestwct -g \"-s cplex \"" -a "columngeneration" -f "row['Dataset'] == 'liao2012_small'"
```

