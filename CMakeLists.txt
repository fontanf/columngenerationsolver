cmake_minimum_required(VERSION 3.15.0)

project(ColumnGenerationSolver LANGUAGES CXX)

# Solver options.
option(COLUMNGENERATIONSOLVER_USE_CLP "Use CLP" ON)
option(COLUMNGENERATIONSOLVER_USE_HIGHS "Use Highs" ON)
option(COLUMNGENERATIONSOLVER_USE_CPLEX "Use CPLEX" OFF)
option(COLUMNGENERATIONSOLVER_USE_XPRESS "Use Xpress" OFF)
option(COLUMNGENERATIONSOLVER_USE_CBC "Use CBC" OFF)

# Other options.
option(COLUMNGENERATIONSOLVER_BUILD_EXAMPLES "Build examples" ON)

# Avoid FetchContent warning.
cmake_policy(SET CMP0135 NEW)

# Require C++14.
set(CMAKE_CXX_STANDARD 14)

# Enable output of compile commands during generation.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set MSVC_RUNTIME_LIBRARY.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Add sub-directories.
add_subdirectory(extern)
add_subdirectory(src)
add_subdirectory(test)
if(COLUMNGENERATIONSOLVER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
