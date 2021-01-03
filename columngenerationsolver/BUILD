cc_library(
        name = "columngenerationsolver",
        hdrs = [
                "columngenerationsolver.hpp",
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
