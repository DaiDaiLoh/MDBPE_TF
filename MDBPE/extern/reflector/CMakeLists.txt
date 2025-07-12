cmake_minimum_required(VERSION 3.8)
project(Reflector)

if (NOT TARGET clean-core)
    message(FATAL_ERROR "[reflector] clean-core must be available")
endif()

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh")

arcana_add_library(RF reflector SOURCES HEADERS)

target_include_directories(reflector PUBLIC src/)

target_link_libraries(reflector PUBLIC
    clean-core
)
