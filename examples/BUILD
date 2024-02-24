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
        name = "cuttingstock",
        hdrs = ["cuttingstock.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@orproblems//orproblems:cuttingstock",
                "@knapsacksolver//knapsacksolver/knapsack/algorithms:dynamic_programming_primal_dual",
                "@knapsacksolver//knapsacksolver/knapsack/algorithms:dynamic_programming_bellman",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "cuttingstock_main",
        srcs = ["cuttingstock_main.cpp"],
        deps = [
                ":cuttingstock",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "multipleknapsack",
        hdrs = ["multipleknapsack.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@orproblems//orproblems:multipleknapsack",
                "@knapsacksolver//knapsacksolver/knapsack/algorithms:dynamic_programming_primal_dual",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "multipleknapsack_main",
        srcs = ["multipleknapsack_main.cpp"],
        deps = [
                ":multipleknapsack",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "binpackingwithconflicts",
        hdrs = ["binpackingwithconflicts.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@orproblems//orproblems:binpackingwithconflicts",
                "@treesearchsolver//examples:knapsackwithconflicts",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "binpackingwithconflicts_main",
        srcs = ["binpackingwithconflicts_main.cpp"],
        deps = [
                ":binpackingwithconflicts",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "capacitatedvehiclerouting",
        hdrs = ["capacitatedvehiclerouting.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@optimizationtools//optimizationtools/utils:utils",
                "@orproblems//orproblems:capacitatedvehiclerouting",
                "//examples/pricingsolver:espprc",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "capacitatedvehiclerouting_main",
        srcs = ["capacitatedvehiclerouting_main.cpp"],
        deps = [
                ":capacitatedvehiclerouting",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "vehicleroutingwithtimewindows",
        hdrs = ["vehicleroutingwithtimewindows.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:espprctw",
                "@optimizationtools//optimizationtools/utils:utils",
                "@orproblems//orproblems:vehicleroutingwithtimewindows",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "vehicleroutingwithtimewindows_main",
        srcs = ["vehicleroutingwithtimewindows_main.cpp"],
        deps = [
                ":vehicleroutingwithtimewindows",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)
