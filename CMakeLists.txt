cmake_minimum_required(VERSION 3.15.0)

project(ColumnGenerationSolver LANGUAGES CXX)

option(COLUMNGENERATIONSOLVER_USE_CLP "Use CLP" ON)
option(COLUMNGENERATIONSOLVER_USE_CPLEX "Use CPLEX" OFF)
option(COLUMNGENERATIONSOLVER_USE_XPRESS "Use Xpress" OFF)

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
