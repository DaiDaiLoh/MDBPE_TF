cmake_minimum_required(VERSION 3.0)
project(CTracer)

if (NOT TARGET clean-core)
    message(FATAL_ERROR "[${PROJECT_NAME}] clean-core must be available")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh")

arcana_add_library(CTRACER ctracer SOURCES HEADERS)

target_include_directories(ctracer PUBLIC "src")

target_link_libraries(ctracer PUBLIC
    clean-core
)
