add_library(ColumnGenerationSolver_read_args INTERFACE)
target_link_libraries(ColumnGenerationSolver_read_args INTERFACE
    ColumnGenerationSolver_columngenerationsolver)
target_include_directories(ColumnGenerationSolver_read_args INTERFACE
    ${PROJECT_SOURCE_DIR}/include)


add_subdirectory(pricingsolver)


add_library(ColumnGenerationSolver_example_cutting_stock INTERFACE)
target_link_libraries(ColumnGenerationSolver_example_cutting_stock INTERFACE
    ColumnGenerationSolver_read_args
    ORProblems::cutting_stock
    KnapsackSolver::knapsack::dynamic_programming_bellman
    KnapsackSolver::knapsack::dynamic_programming_primal_dual)
target_include_directories(ColumnGenerationSolver_example_cutting_stock INTERFACE
    ${PROJECT_SOURCE_DIR}/include)
add_library(ColumnGenerationSolver::examples::cutting_stock ALIAS ColumnGenerationSolver_example_cutting_stock)

add_executable(ColumnGenerationSolver_example_cutting_stock_main)
target_sources(ColumnGenerationSolver_example_cutting_stock_main PRIVATE
    cutting_stock_main.cpp)
target_link_libraries(ColumnGenerationSolver_example_cutting_stock_main PUBLIC
    ColumnGenerationSolver_example_cutting_stock
    Boost::program_options)
set_target_properties(ColumnGenerationSolver_example_cutting_stock_main PROPERTIES OUTPUT_NAME "columngenerationsolver_cutting_stock")
install(TARGETS ColumnGenerationSolver_example_cutting_stock_main)


add_library(ColumnGenerationSolver_example_multiple_knapsack INTERFACE)
target_link_libraries(ColumnGenerationSolver_example_multiple_knapsack INTERFACE
    ColumnGenerationSolver_read_args
    ORProblems::multiple_knapsack
    KnapsackSolver::knapsack::dynamic_programming_primal_dual)
target_include_directories(ColumnGenerationSolver_example_multiple_knapsack INTERFACE
    ${PROJECT_SOURCE_DIR}/include)
add_library(ColumnGenerationSolver::examples::multiple_knapsack ALIAS ColumnGenerationSolver_example_multiple_knapsack)

add_executable(ColumnGenerationSolver_example_multiple_knapsack_main)
target_sources(ColumnGenerationSolver_example_multiple_knapsack_main PRIVATE
    multiple_knapsack_main.cpp)
target_link_libraries(ColumnGenerationSolver_example_multiple_knapsack_main PUBLIC
    ColumnGenerationSolver_example_multiple_knapsack
    Boost::program_options)
set_target_properties(ColumnGenerationSolver_example_multiple_knapsack_main PROPERTIES OUTPUT_NAME "columngenerationsolver_multiple_knapsack")
install(TARGETS ColumnGenerationSolver_example_multiple_knapsack_main)


add_library(ColumnGenerationSolver_example_bin_packing_with_conflicts INTERFACE)
target_link_libraries(ColumnGenerationSolver_example_bin_packing_with_conflicts INTERFACE
    ColumnGenerationSolver_read_args
    ORProblems::bin_packing_with_conflicts
    TreeSearchSolver_example_knapsack_with_conflicts)
target_include_directories(ColumnGenerationSolver_example_bin_packing_with_conflicts INTERFACE
    ${PROJECT_SOURCE_DIR}/include)
add_library(ColumnGenerationSolver::examples::bin_packing_with_conflicts ALIAS ColumnGenerationSolver_example_bin_packing_with_conflicts)

add_executable(ColumnGenerationSolver_example_bin_packing_with_conflicts_main)
target_sources(ColumnGenerationSolver_example_bin_packing_with_conflicts_main PRIVATE
    bin_packing_with_conflicts_main.cpp)
target_link_libraries(ColumnGenerationSolver_example_bin_packing_with_conflicts_main PUBLIC
    ColumnGenerationSolver_example_bin_packing_with_conflicts
    Boost::program_options)
set_target_properties(ColumnGenerationSolver_example_bin_packing_with_conflicts_main PROPERTIES OUTPUT_NAME "columngenerationsolver_bin_packing_with_conflicts")
install(TARGETS ColumnGenerationSolver_example_bin_packing_with_conflicts_main)


add_library(ColumnGenerationSolver_example_capacitated_vehicle_routing INTERFACE)
target_link_libraries(ColumnGenerationSolver_example_capacitated_vehicle_routing INTERFACE
    ColumnGenerationSolver_read_args
    ORProblems::capacitated_vehicle_routing
    ColumnGenerationSolver_example_pricingsolver_espprc)
target_include_directories(ColumnGenerationSolver_example_capacitated_vehicle_routing INTERFACE
    ${PROJECT_SOURCE_DIR}/include)
add_library(ColumnGenerationSolver::examples::capacitated_vehicle_routing ALIAS ColumnGenerationSolver_example_capacitated_vehicle_routing)

add_executable(ColumnGenerationSolver_example_capacitated_vehicle_routing_main)
target_sources(ColumnGenerationSolver_example_capacitated_vehicle_routing_main PRIVATE
    capacitated_vehicle_routing_main.cpp)
target_link_libraries(ColumnGenerationSolver_example_capacitated_vehicle_routing_main PUBLIC
    ColumnGenerationSolver_example_capacitated_vehicle_routing
    Boost::program_options)
set_target_properties(ColumnGenerationSolver_example_capacitated_vehicle_routing_main PROPERTIES OUTPUT_NAME "columngenerationsolver_capacitated_vehicle_routing")
install(TARGETS ColumnGenerationSolver_example_capacitated_vehicle_routing_main)


add_library(ColumnGenerationSolver_example_vehicle_routing_with_time_windows INTERFACE)
target_link_libraries(ColumnGenerationSolver_example_vehicle_routing_with_time_windows INTERFACE
    ColumnGenerationSolver_read_args
    ORProblems::vehicle_routing_with_time_windows
    ColumnGenerationSolver_example_pricingsolver_espprctw)
target_include_directories(ColumnGenerationSolver_example_vehicle_routing_with_time_windows INTERFACE
    ${PROJECT_SOURCE_DIR}/include)
add_library(ColumnGenerationSolver::examples::vehicle_routing_with_time_windows ALIAS ColumnGenerationSolver_example_vehicle_routing_with_time_windows)

add_executable(ColumnGenerationSolver_example_vehicle_routing_with_time_windows_main)
target_sources(ColumnGenerationSolver_example_vehicle_routing_with_time_windows_main PRIVATE
    vehicle_routing_with_time_windows_main.cpp)
target_link_libraries(ColumnGenerationSolver_example_vehicle_routing_with_time_windows_main PUBLIC
    ColumnGenerationSolver_example_vehicle_routing_with_time_windows
    Boost::program_options)
set_target_properties(ColumnGenerationSolver_example_vehicle_routing_with_time_windows_main PROPERTIES OUTPUT_NAME "columngenerationsolver_vehicle_routing_with_time_windows")
install(TARGETS ColumnGenerationSolver_example_vehicle_routing_with_time_windows_main)
