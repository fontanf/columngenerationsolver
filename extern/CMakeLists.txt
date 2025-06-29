# Enable FetchContent.
include(FetchContent)

# Fetch boost.
if (COLUMNGENERATIONSOLVER_BUILD_TEST OR COLUMNGENERATIONSOLVER_BUILD_EXAMPLES)
    set(BOOST_INCLUDE_LIBRARIES thread filesystem system program_options dynamic_bitset)
    set(BOOST_ENABLE_CMAKE ON)
    FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(Boost)
endif()

# Fetch googletest.
if(COLUMNGENERATIONSOLVER_BUILD_TEST)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip)
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF)
    FetchContent_MakeAvailable(googletest)
endif()

# Fetch fontanf/optimizationtools.
set(OPTIMIZATIONTOOLS_BUILD_TEST OFF)
FetchContent_Declare(
    optimizationtools
    GIT_REPOSITORY https://github.com/fontanf/optimizationtools.git
    GIT_TAG e086ec4cfcc9a885cd63f579a99ea5e8f4737005
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../optimizationtools/"
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(optimizationtools)

# Fetch fontanf/mathoptsolverscmake.
if(COLUMNGENERATIONSOLVER_USE_CLP)
    set(MATHOPTSOLVERSCMAKE_USE_CLP ON)
endif()
if(COLUMNGENERATIONSOLVER_USE_CPLEX)
    set(MATHOPTSOLVERSCMAKE_USE_CPLEX ON)
endif()
if(COLUMNGENERATIONSOLVER_USE_XPRESS)
    set(MATHOPTSOLVERSCMAKE_USE_XPRESS ON)
endif()
FetchContent_Declare(
    mathoptsolverscmake
    GIT_REPOSITORY https://github.com/fontanf/mathoptsolverscmake.git
    GIT_TAG f98a9547657a6917659a5397d2f3bb54074293c2
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../mathoptsolverscmake/"
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(mathoptsolverscmake)

# Fetch Highs.
if(COLUMNGENERATIONSOLVER_USE_HIGHS)
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_Declare(
        highs
        URL https://github.com/ERGO-Code/HiGHS/archive/refs/tags/v1.7.2.zip
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(highs)
endif()

# Fetch fontanf/knapsacksolver.
set(KNAPSACKSOLVER_BUILD_MAIN OFF)
set(KNAPSACKSOLVER_BUILD_TEST OFF)
FetchContent_Declare(
    knapsacksolver
    GIT_REPOSITORY https://github.com/fontanf/knapsacksolver.git
    GIT_TAG 5aee5d72a09fb228033d695fe22dc90dba65ec05
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../knapsacksolver/"
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(knapsacksolver)

# Fetch fontanf/treesearchsolver.
set(TREESEARCHSOLVER_BUILD_TEST OFF)
set(TREESEARCHSOLVER_BUILD_EXAMPLES OFF)
FetchContent_Declare(
    treesearchsolver
    GIT_REPOSITORY https://github.com/fontanf/treesearchsolver.git
    GIT_TAG 8cf4a179d103efb9d4e0a94bc2f6f669105cd45a
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../treesearchsolver/"
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(treesearchsolver)
