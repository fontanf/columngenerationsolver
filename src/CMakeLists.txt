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
target_include_directories(ColumnGenerationSolver_columngenerationsolver PRIVATE
    ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(ColumnGenerationSolver_columngenerationsolver PUBLIC
    OptimizationTools::containers
    OptimizationTools::utils)
if(COLUMNGENERATIONSOLVER_USE_CLP)
    target_compile_definitions(ColumnGenerationSolver_columngenerationsolver PUBLIC
        CLP_FOUND=1)
    target_link_libraries(ColumnGenerationSolver_columngenerationsolver PUBLIC
        MathOptSolversCMake::clp)
endif()
if(COLUMNGENERATIONSOLVER_USE_HIGHS)
    target_compile_definitions(ColumnGenerationSolver_columngenerationsolver PUBLIC
        HIGHS_FOUND=1)
    target_link_libraries(ColumnGenerationSolver_columngenerationsolver PUBLIC
        highs)
endif()
if(COLUMNGENERATIONSOLVER_USE_XPRESS)
    target_compile_definitions(ColumnGenerationSolver_columngenerationsolver PUBLIC
        XPRESS_FOUND=1)
    target_link_libraries(ColumnGenerationSolver_columngenerationsolver PUBLIC
        MathOptSolversCMake::xpress)
endif()
if(COLUMNGENERATIONSOLVER_USE_CPLEX)
    target_compile_definitions(ColumnGenerationSolver_columngenerationsolver PUBLIC
        CPLEX_FOUND=1)
    target_link_libraries(ColumnGenerationSolver_columngenerationsolver PUBLIC
        MathOptSolversCMake::cplex)
endif()
add_library(ColumnGenerationSolver::columngenerationsolver ALIAS ColumnGenerationSolver_columngenerationsolver)
set_target_properties(ColumnGenerationSolver_columngenerationsolver PROPERTIES OUTPUT_NAME "columngenerationsolver")
install(TARGETS ColumnGenerationSolver_columngenerationsolver)
