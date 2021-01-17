cc_library(
        name = "columngenerationsolver",
        hdrs = [
                "commons.hpp",
                "linear_programming_solver.hpp",
                "algorithms/column_generation.hpp",
                "algorithms/greedy.hpp",
                "algorithms/limited_discrepancy_search.hpp",
                "algorithms/heuristic_tree_search.hpp",
        ],
        deps = [
                "@optimizationtools//optimizationtools:info",
        ],
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
