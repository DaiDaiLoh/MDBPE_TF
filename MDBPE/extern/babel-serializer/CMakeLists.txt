cmake_minimum_required(VERSION 3.8)
project(BabelSerializer)

if (NOT TARGET clean-core)
    message(FATAL_ERROR "[babel-serializer] clean-core must be available")
endif()

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh")

file(GLOB_RECURSE EXTERN_SOURCES "extern/*.cc" "extern/*.c")
file(GLOB_RECURSE EXTERN_HEADERS "extern/*.h")

arcana_add_library(BABEL babel-serializer SOURCES HEADERS)

target_sources(babel-serializer PRIVATE ${EXTERN_SOURCES} ${EXTERN_HEADERS})

target_include_directories(babel-serializer PUBLIC src/)
target_include_directories(babel-serializer PRIVATE extern/)

target_link_libraries(babel-serializer PUBLIC
    clean-core
    typed-geometry
    rich-log
    reflector
)
