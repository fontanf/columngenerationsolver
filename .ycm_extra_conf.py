def Settings(**kwargs):
    return {
            'flags': [
                '-x', 'c++',
                '-Wall', '-Wextra', '-Werror',
                '-I', '.',

                '-I', './bazel-columngenerationsolver/',
                '-I', './bazel-columngenerationsolver/external/'
                'json/single_include/',
                '-I', './bazel-columngenerationsolver/external/'
                'pugixml/pugixml-1.11/src/',
                '-I', './bazel-columngenerationsolver/external/'
                'googletest/googletest/include/',
                '-I', './bazel-columngenerationsolver/external/boost/',

                # CLP
                '-DCOINOR_FOUND',
                '-I', '/home/florian/Programmes/coinbrew/dist/include/',

                # Gurobi
                '-DGUROBI_FOUND',
                '-I', '/home/florian/Programmes/gurobi811/linux64/include/',

                # Cplex
                '-DCPLEX_FOUND',
                '-DIL_STD',
                '-I', '/opt/ibm/ILOG/CPLEX_Studio129/concert/include/',
                '-I', '/opt/ibm/ILOG/CPLEX_Studio129/cplex/include/',
                '-I', '/opt/ibm/ILOG/CPLEX_Studio129/cpoptimizer/include/',

                # Xpress
                '-I', '/opt/xpressmp/include/',

                # Knitro
                '-DKNITRO_FOUND',
                '-I', '/home/florian/Programmes/'
                'knitro-13.0.0-z-Linux-Intel17-64/include/',

                # Knapsack solver
                # '-I', '../'
                '-I', './bazel-columngenerationsolver/external/'
                'knapsacksolver/',

                # Optimization tools
                # '-I', '../'
                '-I', './bazel-columngenerationsolver/external/'
                'optimizationtools/',

                # OR Problems
                # '-I', '../'
                '-I', './bazel-columngenerationsolver/external/'
                'orproblems/',

                # Tree Search Solver
                # '-I', '../'
                '-I', './bazel-columngenerationsolver/external/'
                'treesearchsolver/',

                # Local Search Solver
                # '-I', '../'
                '-I', './bazel-columngenerationsolver/external/'
                'localsearchsolver/',
                ],
            }
