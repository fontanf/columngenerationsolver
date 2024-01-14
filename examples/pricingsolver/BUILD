cc_library(
        name = "espprc",
        hdrs = ["espprc.hpp"],
        deps = [
                "@optimizationtools//optimizationtools/utils:utils",
                "@optimizationtools//optimizationtools/containers:containers",
                "@treesearchsolver//treesearchsolver:treesearchsolver",
        ],
        visibility = ["//visibility:public"],
)

cc_library(
        name = "espprctw",
        hdrs = ["espprctw.hpp"],
        deps = [
                "@optimizationtools//optimizationtools/utils:utils",
                "@optimizationtools//optimizationtools/containers:containers",
                "@treesearchsolver//treesearchsolver:treesearchsolver",
                "@localsearchsolver//localsearchsolver:localsearchsolver",
                "@boost//:dynamic_bitset",
        ],
        linkopts = ["-pthread"],
        visibility = ["//visibility:public"],
)
