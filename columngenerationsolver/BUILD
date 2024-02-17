load("//columngenerationsolver:variables.bzl", "STDCPP")
load("//columngenerationsolver:variables.bzl", "CLP_DEP", "CLP_COPTS")
load("//columngenerationsolver:variables.bzl", "CPLEX_DEP", "CPLEX_COPTS")
load("//columngenerationsolver:variables.bzl", "XPRESS_DEP", "XPRESS_COPTS")
load("//columngenerationsolver:variables.bzl", "GUROBI_DEP", "GUROBI_COPTS")

cc_library(
        name = "columngenerationsolver",
        hdrs = [
                "commons.hpp",
                "algorithm_formatter.hpp",
                "linear_programming_solver.hpp",
                "algorithms/column_generation.hpp",
                "algorithms/greedy.hpp",
                "algorithms/limited_discrepancy_search.hpp",
                "algorithms/heuristic_tree_search.hpp",
        ],
        srcs = [
                "algorithm_formatter.cpp",
                "algorithms/column_generation.cpp",
                "algorithms/greedy.cpp",
                "algorithms/limited_discrepancy_search.cpp",
                "algorithms/heuristic_tree_search.cpp",
        ],
        deps = [
                "@optimizationtools//optimizationtools/utils:utils",
                "@optimizationtools//optimizationtools/containers:containers",
        ] + CLP_DEP + CPLEX_DEP + GUROBI_DEP + XPRESS_DEP,
        copts = STDCPP + CLP_COPTS + CPLEX_COPTS + GUROBI_COPTS + XPRESS_COPTS,
        visibility = ["//visibility:public"],
)

cc_library(
        name = "read_args",
        hdrs = ["read_args.hpp"],
        deps = [
                "columngenerationsolver",
                "@boost//:program_options",
        ],
        visibility = ["//visibility:public"],
)
