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
FetchContent_Declare(
    optimizationtools
    GIT_REPOSITORY https://github.com/fontanf/optimizationtools.git
    GIT_TAG 4037a1a03f97ea0c388baa4f3f74c3ba55baec08
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
    GIT_TAG 4472814a28a40f4d861ccd757e140835c3a31bd0
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
FetchContent_Declare(
    knapsacksolver
    GIT_REPOSITORY https://github.com/fontanf/knapsacksolver.git
    GIT_TAG abd1acf0704744f5e975e75a1f09cb5a9679e7e8
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../knapsacksolver/"
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(knapsacksolver)

# Fetch fontanf/treesearchsolver.
FetchContent_Declare(
    treesearchsolver
    GIT_REPOSITORY https://github.com/fontanf/treesearchsolver.git
    GIT_TAG 8e2d5190d4005a753a815641b1e3f8753a9d7095
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../treesearchsolver/"
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(treesearchsolver)
