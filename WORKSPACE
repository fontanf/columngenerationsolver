load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "googletest",
    remote = "https://github.com/google/googletest.git",
    commit = "58d77fa8070e8cec2dc1ed015d66b454c8d78850",
    shallow_since = "1656350095 -0400",
)

git_repository(
    name = "com_github_nelhage_rules_boost",
    remote = "https://github.com/nelhage/rules_boost",
    commit = "e83dfef18d91a3e35c8eac9b9aeb1444473c0efd",
    shallow_since = "1671181466 +0000",
)
load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()

http_archive(
    name = "json",
    build_file_content = """
cc_library(
        name = "json",
        hdrs = ["single_include/nlohmann/json.hpp"],
        visibility = ["//visibility:public"],
        strip_include_prefix = "single_include/"
)
""",
    urls = ["https://github.com/nlohmann/json/releases/download/v3.7.3/include.zip"],
    sha256 = "87b5884741427220d3a33df1363ae0e8b898099fbc59f1c451113f6732891014",
)

git_repository(
    name = "optimizationtools",
    remote = "https://github.com/fontanf/optimizationtools.git",
    commit = "d7299a707f3b6b3b8aa8c428f5254b9ea39bfc53",
    shallow_since = "1656765328 +0200",
)

local_repository(
    name = "optimizationtools_",
    path = "../optimizationtools/",
)

git_repository(
    name = "orproblems",
    remote = "https://github.com/fontanf/orproblems.git",
    commit = "53644e340ba65d1c880ec8d7d421a08ac6364186",
    shallow_since = "1672482002 +0100",
)

local_repository(
    name = "orproblems_",
    path = "../orproblems/",
)

git_repository(
    name = "knapsacksolver",
    remote = "https://github.com/fontanf/knapsacksolver.git",
    commit = "abb0b7891b8be06948c4eda328ba2134eebfb6e5",
    shallow_since = "1670663487 +0100",
)

local_repository(
    name = "knapsacksolver_",
    path = "../knapsacksolver/",
)

git_repository(
    name = "treesearchsolver",
    remote = "https://github.com/fontanf/treesearchsolver.git",
    commit = "c39a99bb46ef73f4bd092460517d530f3a273569",
    shallow_since = "1655023961 +0200",
)

local_repository(
    name = "treesearchsolver_",
    path = "../treesearchsolver/",
)

git_repository(
    name = "localsearchsolver_",
    remote = "https://github.com/fontanf/localsearchsolver.git",
    commit = "077f3565fae1ea52f4ba7f07347b33925c54f380",
)

local_repository(
    name = "localsearchsolver",
    path = "../localsearchsolver/",
)

new_local_repository(
    name = "coinor",
    path = "/home/florian/Programmes/coinbrew/",
    build_file_content = """
cc_library(
    name = "osi",
    hdrs = glob(["dist/include/coin/Osi*.h*"], exclude_directories = 0),
    strip_include_prefix = "dist/include/coin/",
    visibility = ["//visibility:public"],
)
cc_library(
    name = "coinutils",
    hdrs = glob(["dist/include/coin/Coin*.h*"], exclude_directories = 0),
    strip_include_prefix = "dist/include/coin/",
    srcs = [
        "dist/lib/libCoinUtils.so",
    ],
    visibility = ["//visibility:public"],
)
cc_library(
    name = "clp",
    hdrs = glob(["dist/include/coin/Clp*.h*"], exclude_directories = 0),
    strip_include_prefix = "dist/include/coin",
    srcs = [
        "dist/lib/libClp.so",
    ],
    deps = [":coinutils", ":osi"],
    visibility = ["//visibility:public"],
)
""",
)

new_local_repository(
    name = "cplex",
    path = "/opt/ibm/ILOG/CPLEX_Studio129/",
    build_file_content = """
cc_library(
    name = "concert",
    hdrs = glob(["concert/include/ilconcert/**/*.h"], exclude_directories = 0),
    strip_include_prefix = "concert/include/",
    srcs = ["concert/lib/x86-64_linux/static_pic/libconcert.a"],
    linkopts = [
            "-lm",
            "-lpthread",
            "-ldl",
    ],
    visibility = ["//visibility:public"],
)
cc_library(
    name = "cplex",
    hdrs = glob(["cplex/include/ilcplex/*.h"]),
    strip_include_prefix = "cplex/include/",
    srcs = [
            "cplex/lib/x86-64_linux/static_pic/libilocplex.a",
            "cplex/lib/x86-64_linux/static_pic/libcplex.a",
    ],
    deps = [":concert"],
    visibility = ["//visibility:public"],
)
cc_library(
    name = "cpoptimizer",
    hdrs = glob(["cpoptimizer/include/ilcp/*.h"]),
    strip_include_prefix = "cpoptimizer/include/",
    srcs = ["cpoptimizer/lib/x86-64_linux/static_pic/libcp.a"],
    deps = [":cplex"],
    visibility = ["//visibility:public"],
)
""",
)

new_local_repository(
    name = "gurobi",
    path = "/home/florian/Programmes/gurobi811/linux64/",
    build_file_content = """
cc_library(
    name = "gurobi",
    hdrs = [
            "include/gurobi_c.h",
            "include/gurobi_c++.h",
    ],
    strip_include_prefix = "include/",
    srcs = [
            "lib/libgurobi_c++.a",
            "lib/libgurobi81.so",
    ],
    visibility = ["//visibility:public"],
)
""",
)

new_local_repository(
    name = "xpress",
    path = "/opt/xpressmp/",
    build_file_content = """
cc_library(
    name = "xpress",
    hdrs = glob(["include/*.h"], exclude_directories = 0),
    strip_include_prefix = "include/",
    srcs = ["lib/libxprs.so"],
    visibility = ["//visibility:public"],
)
""",
)

new_local_repository(
    name = "knitro",
    path = "/home/florian/Programmes/knitro-13.0.1-Linux-64//",
    build_file_content = """
cc_library(
    name = "knitro",
    hdrs = [
            "include/knitro.h",
    ],
    strip_include_prefix = "include/",
    srcs = [
            "lib/libknitro.so",
            "lib/libiomp5.so",
    ],
    copts = [
            "-fopenmp",
    ],
    linkopts = [
            "-fopenmp",
            "-ldl",
            "-liomp5",
    ],
    visibility = ["//visibility:public"],
)
""",
)

