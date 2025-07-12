cmake_minimum_required(VERSION 3.11)

file(GLOB_RECURSE SOURCES
    "*.cc"
    "*.hh"
)

add_arcana_test(tests-babel-serializer "${SOURCES}")

target_include_directories(tests-babel-serializer PUBLIC ./)

target_link_libraries(tests-babel-serializer PUBLIC
    clean-core
    clean-ranges
    typed-geometry
    babel-serializer
)
