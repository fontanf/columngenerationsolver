config_setting(name = "coinor_build", values = {"define": "coinor=true"}, visibility = ["//visibility:public"])
config_setting(name = "cplex_build",  values = {"define": "cplex=true"},  visibility = ["//visibility:public"])
config_setting(name = "xpress_build", values = {"define": "xpress=true"}, visibility = ["//visibility:public"])
config_setting(name = "gurobi_build", values = {"define": "gurobi=true"}, visibility = ["//visibility:public"])
config_setting(name = "knitro_build", values = {"define": "knitro=true"}, visibility = ["//visibility:public"])

load("//examples:variables.bzl", "STDCPP",
         "COINOR_COPTS", "COINOR_DEP",
         "CPLEX_COPTS", "CPLEX_DEP",
         "XPRESS_COPTS", "XPRESS_DEP",
         "GUROBI_COPTS", "GUROBI_DEP",
         "KNITRO_COPTS", "KNITRO_DEP")

cc_library(
        name = "cuttingstock",
        hdrs = ["cuttingstock.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@knapsacksolver//knapsacksolver/algorithms:minknap",
        ],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "multipleknapsack",
        hdrs = ["multipleknapsack.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@knapsacksolver//knapsacksolver/algorithms:minknap",
        ],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "binpackingwithconflicts",
        hdrs = ["binpackingwithconflicts.hpp"],
        deps = [
                "//columngenerationsolver:columngenerationsolver",
                "@treesearchsolver//examples:knapsackwithconflicts",
        ],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "capacitatedvehiclerouting",
        hdrs = ["capacitatedvehiclerouting.hpp"],
        deps = [
                "@optimizationtools//optimizationtools:utils",
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:espprc",
        ],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "vehicleroutingwithtimewindows",
        hdrs = ["vehicleroutingwithtimewindows.hpp"],
        deps = [
                "@pugixml//:pugixml",
                "@optimizationtools//optimizationtools:utils",
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:espprctw",
        ],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "capacitatedopenvehiclerouting",
        hdrs = ["capacitatedopenvehiclerouting.hpp"],
        deps = [
                "@optimizationtools//optimizationtools:utils",
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:eospprc",
        ],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "parallelschedulingwithfamilysetuptimestwct",
        hdrs = ["parallelschedulingwithfamilysetuptimestwct.hpp"],
        deps = [
                "@optimizationtools//optimizationtools:utils",
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:oaschedulingwithfamilysetuptimestwct",
        ],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "starobservationscheduling",
        hdrs = ["starobservationscheduling.hpp"],
        deps = [
                "@optimizationtools//optimizationtools:utils",
                "//columngenerationsolver:columngenerationsolver",
                "//examples/pricingsolver:singlenightstarobservationscheduling",
        ],
        visibility = ["//visibility:public"],
)

cc_binary(
        name = "main",
        srcs = ["main.cpp"],
        deps = [
                ":cuttingstock",
                ":multipleknapsack",
                ":binpackingwithconflicts",
                ":capacitatedvehiclerouting",
                ":vehicleroutingwithtimewindows",
                ":capacitatedopenvehiclerouting",
                ":parallelschedulingwithfamilysetuptimestwct",
                ":starobservationscheduling",
                "//columngenerationsolver:read_args",
                "@boost//:program_options",
        ] + COINOR_DEP + XPRESS_DEP + CPLEX_DEP + GUROBI_DEP + KNITRO_DEP,
        copts = COINOR_COPTS + XPRESS_COPTS + CPLEX_COPTS + GUROBI_COPTS + KNITRO_COPTS,
)

