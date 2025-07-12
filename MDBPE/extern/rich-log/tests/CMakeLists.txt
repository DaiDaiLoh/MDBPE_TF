cmake_minimum_required(VERSION 3.11)

file(GLOB_RECURSE SOURCES
    "*.cc"
    "*.hh"
)

add_arcana_test(tests-rich-log "${SOURCES}")

target_link_libraries(tests-rich-log PUBLIC
    clean-core
    clean-ranges
    typed-geometry
    rich-log
)
