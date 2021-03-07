load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "googletest",
    remote = "https://github.com/google/googletest.git",
    commit = "703bd9caab50b139428cea1aaff9974ebee5742e",
    shallow_since = "1570114335 -0400",
)

git_repository(
    name = "com_github_nelhage_rules_boost",
    commit = "9f9fb8b2f0213989247c9d5c0e814a8451d18d7f",
    remote = "https://github.com/nelhage/rules_boost",
    shallow_since = "1570056263 -0700",
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

http_archive(
    name = "pugixml",
    build_file_content = """
cc_library(
        name = "pugixml",
        srcs = ["pugixml-1.11/src/pugixml.cpp"],
        hdrs = ["pugixml-1.11/src/pugixml.hpp", "pugixml-1.11/src/pugiconfig.hpp"],
        visibility = ["//visibility:public"],
        strip_include_prefix = "pugixml-1.11/src/"
)
""",
    urls = ["https://github.com/zeux/pugixml/releases/download/v1.11/pugixml-1.11.tar.gz"],
    sha256 = "26913d3e63b9c07431401cf826df17ed832a20d19333d043991e611d23beaa2c",
)

git_repository(
    name = "optimizationtools",
    remote = "https://github.com/fontanf/optimizationtools.git",
    commit = "09c0a144b914b4aac539120a54d989680fd96663",
    shallow_since = "1613237666 +0100",
)

local_repository(
    name = "optimizationtools_",
    path = "../optimizationtools/",
)

git_repository(
    name = "knapsacksolver",
    remote = "https://github.com/fontanf/knapsacksolver.git",
    commit = "bc4ffc72a301c9aa033b969ff54d3c3396de2b8f",
    shallow_since = "1609063241 +0100",
)

git_repository(
    name = "treesearchsolver",
    remote = "https://github.com/fontanf/treesearchsolver.git",
    commit = "f732bd43f0326dde3e3a1e4481d3fa593cce0aca",
    shallow_since = "1613237903 +0100",
)

local_repository(
    name = "treesearchsolver_",
    path = "../treesearchsolver/",
)

new_local_repository(
    name = "coinor",
    path = "/home/florian/Programmes/coinbrew/",
    build_file_content = """
cc_library(
    name = "coinor",
    hdrs = glob(["dist/include/**/*.h*"], exclude_directories = 0),
    strip_include_prefix = "dist/include/",
    srcs = glob(["dist/lib/**/*.so"], exclude_directories = 0),
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
    path = "",
    build_file_content = """
cc_library(
    name = "xpress",
    visibility = ["//visibility:public"],
)
""",
)

new_local_repository(
    name = "knitro",
    path = "/home/florian/Programmes/knitro-13.0.0-z-Linux-Intel17-64/",
    build_file_content = """
cc_library(
    name = "knitro",
    hdrs = [
            "include/knitro.h",
            "include/ktr.h",
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

