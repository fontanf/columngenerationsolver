cc_library(
        name = "espprc",
        hdrs = ["espprc.hpp"],
        deps = [
                "@optimizationtools//optimizationtools/utils:info",
                "@optimizationtools//optimizationtools/containers:containers",
                "@treesearchsolver//treesearchsolver:treesearchsolver",
        ],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "espprctw",
        hdrs = ["espprctw.hpp"],
        deps = [
                "@optimizationtools//optimizationtools/utils:info",
                "@optimizationtools//optimizationtools/containers:containers",
                "@treesearchsolver//treesearchsolver:treesearchsolver",
                "@localsearchsolver//localsearchsolver:localsearchsolver",
                "@boost//:dynamic_bitset",
        ],
        linkopts = ["-pthread"],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "singlenightstarobservationscheduling",
        hdrs = ["singlenightstarobservationscheduling.hpp"],
        deps = [
                "@optimizationtools//optimizationtools/utils:info",
        ],
        visibility = ["//visibility:public"],
)
