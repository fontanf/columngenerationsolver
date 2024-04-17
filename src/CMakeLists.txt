add_library(ColumnGenerationSolver_columngenerationsolver)
target_sources(ColumnGenerationSolver_columngenerationsolver PRIVATE
    commons.cpp
    algorithm_formatter.cpp
    algorithms/column_generation.cpp
    algorithms/greedy.cpp
    algorithms/limited_discrepancy_search.cpp
    algorithms/heuristic_tree_search.cpp)
target_include_directories(ColumnGenerationSolver_columngenerationsolver PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ColumnGenerationSolver_columngenerationsolver PUBLIC
    OptimizationTools::containers
    OptimizationTools::utils
    MathOptSolversCMake::clp)
add_library(ColumnGenerationSolver::columngenerationsolver ALIAS ColumnGenerationSolver_columngenerationsolver)
set_target_properties(ColumnGenerationSolver_columngenerationsolver PROPERTIES OUTPUT_NAME "columngenerationsolver")
install(TARGETS ColumnGenerationSolver_columngenerationsolver)

add_subdirectory(examples)
