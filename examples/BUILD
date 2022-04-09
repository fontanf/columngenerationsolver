config_setting(name = "coinor_build", values = {"define": "coinor=true"}, visibility = ["//visibility:public"])
config_setting(name = "cplex_build",  values = {"define": "cplex=true"},  visibility = ["//visibility:public"])
config_setting(name = "xpress_build", values = {"define": "xpress=true"}, visibility = ["//visibility:public"])
config_setting(name = "gurobi_build", values = {"define": "gurobi=true"}, visibility = ["//visibility:public"])
config_setting(name = "knitro_build", values = {"define": "knitro=true"}, visibility = ["//visibility:public"])

load("//examples:variables.bzl", "STDCPP", "ALL_COPTS", "ALL_DEP")

cc_library(
        name = "cuttingstock",
        hdrs = ["cuttingstock.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@orproblems//orproblems:cuttingstock",
                "@knapsacksolver//knapsacksolver/algorithms:minknap",
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
                "@knapsacksolver//knapsacksolver/algorithms:minknap",
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
                "@pugixml//:pugixml",
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


cc_library(
        name = "capacitatedopenvehiclerouting",
        hdrs = ["capacitatedopenvehiclerouting.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:eospprc",
                "@optimizationtools//optimizationtools/utils:utils",
                "@orproblems//orproblems:capacitatedopenvehiclerouting",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "capacitatedopenvehiclerouting_main",
        srcs = ["capacitatedopenvehiclerouting_main.cpp"],
        deps = [
                ":capacitatedopenvehiclerouting",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "parallelschedulingwithfamilysetuptimestwct",
        hdrs = ["parallelschedulingwithfamilysetuptimestwct.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:oaschedulingwithfamilysetuptimestwct",
                "@optimizationtools//optimizationtools/utils:utils",
                "@orproblems//orproblems:parallelschedulingwithfamilysetuptimestwct",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "parallelschedulingwithfamilysetuptimestwct_main",
        srcs = ["parallelschedulingwithfamilysetuptimestwct_main.cpp"],
        deps = [
                ":parallelschedulingwithfamilysetuptimestwct",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)


cc_library(
        name = "starobservationscheduling",
        hdrs = ["starobservationscheduling.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:singlenightstarobservationscheduling",
                "@optimizationtools//optimizationtools/utils:utils",
                "@orproblems//orproblems:starobservationscheduling",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "starobservationscheduling_main",
        srcs = ["starobservationscheduling_main.cpp"],
        deps = [
                ":starobservationscheduling",
                "//columngenerationsolver:read_args",
        ] + ALL_DEP,
        copts = ALL_COPTS,
)

