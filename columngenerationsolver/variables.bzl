STDCPP = select({
            "@bazel_tools//src/conditions:windows": ['/std:c++latest'],
            "//conditions:default": ["-std=c++11"],})

CLP_COPTS = select({
            "//examples:clp_build": ["-DCLP_FOUND"],
            "//conditions:default": []})
CLP_DEP = select({
            "//examples:clp_windows": ["@clp_windows//:clp"],
            "//conditions:default": []
        }) + select({
            "//examples:clp_linux": ["@clp_linux//:clp"],
            "//conditions:default": []})

CPLEX_COPTS = select({
            "//examples:cplex_build": [
                    "-DCPLEX_FOUND",
                    "-m64",
                    "-DIL_STD"],
            "//conditions:default": []})
CPLEX_DEP = select({
            "//examples:cplex_build": ["@cplex//:cplex"],
            "//conditions:default": []})

GUROBI_COPTS = select({
            "//examples:gurobi_build": ["-DGUROBI_FOUND"],
            "//conditions:default": []})
GUROBI_DEP = select({
            "//examples:gurobi_build": ["@gurobi//:gurobi"],
            "//conditions:default": []})

XPRESS_COPTS = select({
            "//examples:xpress_build": ["-DXPRESS_FOUND"],
            "//conditions:default": []})
XPRESS_DEP = select({
            "//examples:xpress_build": ["@xpress//:xpress"],
            "//conditions:default": []})

KNITRO_COPTS = select({
            "//examples:knitro_build": ["-DKNITRO_FOUND"],
            "//conditions:default": []})
KNITRO_DEP = select({
            "//examples:knitro_build": ["@knitro//:knitro"],
            "//conditions:default": []})

ALL_COPTS = CLP_COPTS + XPRESS_COPTS + CPLEX_COPTS + GUROBI_COPTS + KNITRO_COPTS
ALL_DEP = CLP_DEP + XPRESS_DEP + CPLEX_DEP + GUROBI_DEP + KNITRO_DEP

