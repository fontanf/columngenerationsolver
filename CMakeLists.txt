cmake_minimum_required(VERSION 3.15.0)

project(ColumnGenerationSolver LANGUAGES CXX)

# Require C++14.
set(CMAKE_CXX_STANDARD 14)

# Enable output of compile commands during generation.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add sub-directories.
add_subdirectory(extern)
add_subdirectory(src)
add_subdirectory(test)
