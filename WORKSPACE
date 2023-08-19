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
    commit = "e4b1995bd230a80f2bbfa83ccf8e229db3bb01a6",
)

local_repository(
    name = "optimizationtools_",
    path = "../optimizationtools/",
)

git_repository(
    name = "orproblems",
    remote = "https://github.com/fontanf/orproblems.git",
    commit = "7f3f6662f26961018b9d8f4fc6ec140a94358ddd",
)

local_repository(
    name = "orproblems_",
    path = "../orproblems/",
)

git_repository(
    name = "knapsacksolver",
    remote = "https://github.com/fontanf/knapsacksolver.git",
    commit = "1525b332d352e821723b9509d3e96d405571fe67",
)

local_repository(
    name = "knapsacksolver_",
    path = "../knapsacksolver/",
)

git_repository(
    name = "treesearchsolver",
    remote = "https://github.com/fontanf/treesearchsolver.git",
    commit = "7d3e36252a547879f476a34bbda2777fa249b0d3",
)

local_repository(
    name = "treesearchsolver_",
    path = "../treesearchsolver/",
)

git_repository(
    name = "localsearchsolver",
    remote = "https://github.com/fontanf/localsearchsolver.git",
    commit = "43572d963b158adcaeeda5c64f253f4c5c394969",
)

local_repository(
    name = "localsearchsolver_",
    path = "../localsearchsolver/",
)

http_archive(
    name = "coinor_linux",
    urls = ["https://github.com/coin-or/Cbc/releases/download/releases%2F2.10.10/Cbc-releases.2.10.10-x86_64-ubuntu20-gcc940-static.tar.gz"],
    sha256 = "872c78bfcdd1566f134d2f7757b76b2a2479a5b1ade065cdd1d4b303ed6f8006",
    build_file_content = """
cc_library(
    name = "osi",
    hdrs = glob(["include/coin/Osi*.h*"], exclude_directories = 0),
    strip_include_prefix = "include/coin/",
    visibility = ["//visibility:public"],
)
cc_library(
    name = "coinutils",
    hdrs = glob(["include/coin/Coin*.h*"], exclude_directories = 0),
    strip_include_prefix = "include/coin/",
    srcs = ["lib/libCoinUtils.a"],
    linkopts = ["-llapack", "-lblas", "-lbz2", "-lz"],
    visibility = ["//visibility:public"],
)
cc_library(
    name = "clp",
    hdrs = glob(["include/coin/Clp*.h*"], exclude_directories = 0),
    strip_include_prefix = "include/coin",
    srcs = ["lib/libClp.a"],
    deps = [":coinutils", ":osi"],
    visibility = ["//visibility:public"],
)
cc_library(
    name = "cbc",
    hdrs = glob(["include/coin/Cbc*.h*"], exclude_directories = 0),
    strip_include_prefix = "include/coin",
    srcs = ["lib/libCbc.a", "lib/libOsiCbc.a"],
    deps = [":coinutils", ":osi", ":clp"],
    visibility = ["//visibility:public"],
)
""",
)

http_archive(
    name = "coinor_windows",
    urls = ["https://github.com/coin-or/Cbc/releases/download/releases%2F2.10.10/Cbc-releases.2.10.10-w64-msvc16-md.zip"],
    build_file_content = """
cc_library(
    name = "osi",
    hdrs = glob(["include/coin/Osi*.h*"], exclude_directories = 0),
    strip_include_prefix = "include/coin/",
    visibility = ["//visibility:public"],
    srcs = ["lib/libOsi.lib", "lib/libOsiCommonTests.lib"],
)
cc_library(
    name = "coinutils",
    hdrs = glob(["include/coin/Coin*.h*"], exclude_directories = 0),
    strip_include_prefix = "include/coin/",
    srcs = ["lib/libCoinUtils.lib"],
    visibility = ["//visibility:public"],
)
cc_library(
    name = "clp",
    hdrs = glob(["include/coin/Clp*.h*"], exclude_directories = 0),
    strip_include_prefix = "include/coin",
    srcs = ["lib/libClp.lib", "lib/libOsiClp.lib"],
    deps = [":coinutils", ":osi"],
    visibility = ["//visibility:public"],
)
cc_library(
    name = "cbc",
    hdrs = glob(["include/coin/Cbc*.h*"], exclude_directories = 0),
    strip_include_prefix = "include/coin",
    srcs = ["lib/libCbc.lib", "lib/libOsiCbc.lib", "lib/libCgl.lib"],
    deps = [":coinutils", ":osi", ":clp"],
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

