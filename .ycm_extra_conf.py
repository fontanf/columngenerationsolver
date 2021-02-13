def Settings(**kwargs):
    return {
            'flags': [
                '-x', 'c++',
                '-Wall', '-Wextra', '-Werror',
                '-DCOINOR_FOUND',
                '-DCPLEX_FOUND',
                '-DGUROBI_FOUND',
                '-DGECODE_FOUND',
                '-DIL_STD',  # Cplex
                '-I', '.',
                '-I', '/home/florian/Programmes/coinbrew/dist/include/',
                '-I', '/opt/ibm/ILOG/CPLEX_Studio129/concert/include/',
                '-I', '/opt/ibm/ILOG/CPLEX_Studio129/cplex/include/',
                '-I', '/opt/ibm/ILOG/CPLEX_Studio129/cpoptimizer/include/',
                '-I', '/home/florian/Programmes/gurobi811/linux64/include/',
                '-I', './bazel-columngenerationsolver/external/json/single_include/',
                '-I', './bazel-columngenerationsolver/external/googletest/googletest/include/',
                '-I', './bazel-columngenerationsolver/external/boost/',
                '-I', './bazel-columngenerationsolver/external/knapsacksolver/',
                '-I', './bazel-columngenerationsolver/external/optimizationtools/',
                # '-I', './../optimizationtools/',
                '-I', './bazel-columngenerationsolver/external/treesearchsolver/',
                # '-I', './../treesearchsolver/',
                '-I', './bazel-columngenerationsolver/external/dlib/dlib-19.19/',
                ],
            }
