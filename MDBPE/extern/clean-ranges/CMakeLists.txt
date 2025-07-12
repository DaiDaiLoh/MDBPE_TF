cmake_minimum_required(VERSION 3.8)
project(CleanRanges)

if (NOT TARGET clean-core)
    message(FATAL_ERROR "[clean-ranges] clean-core must be available")
endif()

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh")

arcana_add_library(CR clean-ranges SOURCES HEADERS)

target_include_directories(clean-ranges PUBLIC src/)

target_link_libraries(clean-ranges PUBLIC
    clean-core
    typed-geometry
)
