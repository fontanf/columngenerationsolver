add_library(ColumnGenerationSolver_example_pricingsolver_espprc INTERFACE)
target_link_libraries(ColumnGenerationSolver_example_pricingsolver_espprc INTERFACE
    OptimizationTools::utils
    OptimizationTools::containers
    TreeSearchSolver::treesearchsolver)
target_include_directories(ColumnGenerationSolver_example_pricingsolver_espprc INTERFACE
    ${PROJECT_SOURCE_DIR}/include)

add_library(ColumnGenerationSolver_example_pricingsolver_espprctw INTERFACE)
target_link_libraries(ColumnGenerationSolver_example_pricingsolver_espprctw INTERFACE
    OptimizationTools::utils
    OptimizationTools::containers
    TreeSearchSolver::treesearchsolver
    Boost::dynamic_bitset)
target_include_directories(ColumnGenerationSolver_example_pricingsolver_espprctw INTERFACE
    ${PROJECT_SOURCE_DIR}/include)
