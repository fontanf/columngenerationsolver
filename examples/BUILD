config_setting(name = "coinor_build", values = {"define": "coinor=true"}, visibility = ["//visibility:public"])
config_setting(name = "cplex_build",  values = {"define": "cplex=true"},  visibility = ["//visibility:public"])
config_setting(name = "xpress_build", values = {"define": "xpress=true"}, visibility = ["//visibility:public"])
config_setting(name = "gurobi_build", values = {"define": "gurobi=true"}, visibility = ["//visibility:public"])

load("//examples:variables.bzl", "STDCPP",
         "COINOR_COPTS", "COINOR_DEP",
         "CPLEX_COPTS", "CPLEX_DEP",
         "XPRESS_COPTS", "XPRESS_DEP",
         "GUROBI_COPTS", "GUROBI_DEP")

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
        name = "capacitatedvehiclerouting",
        hdrs = ["capacitatedvehiclerouting.hpp"],
        deps = [
                "@optimizationtools//optimizationtools:utils",
                "//columngenerationsolver:columngenerationsolver",
                #"@knapsacksolver//knapsacksolver/algorithms:minknap",
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
                ":capacitatedvehiclerouting",
                ":starobservationscheduling",
                "//columngenerationsolver:read_args",
                "@boost//:program_options",
        ] + COINOR_DEP + XPRESS_DEP + CPLEX_DEP + GUROBI_DEP,
        copts = COINOR_COPTS + XPRESS_COPTS + CPLEX_COPTS + GUROBI_COPTS,
        data = ["//data:data"],
)

