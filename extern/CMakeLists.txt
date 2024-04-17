# Enable FetchContent.
include(FetchContent)

# Fetch boost.
set(BOOST_INCLUDE_LIBRARIES thread filesystem system program_options dynamic_bitset)
set(BOOST_ENABLE_CMAKE ON)
include(FetchContent)
FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG boost-1.84.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Boost)

# Fetch fontanf/mathoptsolverscmake.
FetchContent_Declare(
    mathoptsolverscmake
    GIT_REPOSITORY https://github.com/fontanf/mathoptsolverscmake.git
    GIT_TAG ca1d280e1bdadf2e4cf39c2f4780df0b1f7d24fd)
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../mathoptsolverscmake/")
FetchContent_MakeAvailable(mathoptsolverscmake)

# Fetch fontanf/knapsacksolver.
FetchContent_Declare(
    knapsacksolver
    GIT_REPOSITORY https://github.com/fontanf/knapsacksolver.git
    GIT_TAG abbe052b56c6e985112cda23abda26f1eb0e43ce)
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../knapsacksolver/")
FetchContent_MakeAvailable(knapsacksolver)

# Fetch fontanf/treesearchsolver.
FetchContent_Declare(
    treesearchsolver
    GIT_REPOSITORY https://github.com/fontanf/treesearchsolver.git
    GIT_TAG dca5b35d5cb6de3443b9ab08175f931b2df29b6b)
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../treesearchsolver/")
FetchContent_MakeAvailable(treesearchsolver)
