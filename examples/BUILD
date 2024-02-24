load("@bazel_skylib//lib:selects.bzl", "selects")

config_setting(
        name = "clp_build",
        values = {"define": "clp=true"},
        visibility = ["//visibility:public"],
)

selects.config_setting_group(
    name = "clp_linux",
    match_all = [":clp_build", "@bazel_tools//src/conditions:linux"],
)

selects.config_setting_group(
    name = "clp_windows",
    match_all = [":clp_build", "@bazel_tools//src/conditions:windows"],
)

config_setting(
        name = "cplex_build",
        values = {"define": "cplex=true"},
        visibility = ["//visibility:public"],
)

config_setting(
        name = "xpress_build",
        values = {"define": "xpress=true"},
        visibility = ["//visibility:public"],
)

config_setting(
        name = "gurobi_build",
        values = {"define": "gurobi=true"},
        visibility = ["//visibility:public"],
)

config_setting(
        name = "knitro_build",
        values = {"define": "knitro=true"},
        visibility = ["//visibility:public"],
)

load("//columngenerationsolver:variables.bzl", "STDCPP", "ALL_COPTS", "ALL_DEP")

cc_library(
        name = "cutting_stock",
        hdrs = ["cutting_stock.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@orproblems//orproblems:cutting_stock",
                "@knapsacksolver//knapsacksolver/knapsack/algorithms:dynamic_programming_primal_dual",
                "@knapsacksolver//knapsacksolver/knapsack/algorithms:dynamic_programming_bellman",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "cutting_stock_main",
        srcs = ["cutting_stock_main.cpp"],
        deps = [
                ":cutting_stock",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "multiple_knapsack",
        hdrs = ["multiple_knapsack.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@orproblems//orproblems:multiple_knapsack",
                "@knapsacksolver//knapsacksolver/knapsack/algorithms:dynamic_programming_primal_dual",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "multiple_knapsack_main",
        srcs = ["multiple_knapsack_main.cpp"],
        deps = [
                ":multiple_knapsack",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "bin_packing_with_conflicts",
        hdrs = ["bin_packing_with_conflicts.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@orproblems//orproblems:bin_packing_with_conflicts",
                "@treesearchsolver//examples:knapsack_with_conflicts",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "bin_packing_with_conflicts_main",
        srcs = ["bin_packing_with_conflicts_main.cpp"],
        deps = [
                ":bin_packing_with_conflicts",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "capacitated_vehicle_routing",
        hdrs = ["capacitated_vehicle_routing.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@optimizationtools//optimizationtools/utils:utils",
                "@orproblems//orproblems:capacitated_vehicle_routing",
                "//examples/pricingsolver:espprc",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "capacitated_vehicle_routing_main",
        srcs = ["capacitated_vehicle_routing_main.cpp"],
        deps = [
                ":capacitated_vehicle_routing",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "vehicle_routing_with_time_windows",
        hdrs = ["vehicle_routing_with_time_windows.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:espprctw",
                "@optimizationtools//optimizationtools/utils:utils",
                "@orproblems//orproblems:vehicle_routing_with_time_windows",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "vehicle_routing_with_time_windows_main",
        srcs = ["vehicle_routing_with_time_windows_main.cpp"],
        deps = [
                ":vehicle_routing_with_time_windows",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)
