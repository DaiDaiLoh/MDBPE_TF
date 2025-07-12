cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE SOURCE_FILES "glow-extras/*.cc")
file(GLOB_RECURSE HEADER_FILES "glow-extras/*.hh")

if (GLOW_EXTRAS_DEFAULT_FONTS)
    list(APPEND SOURCE_FILES 
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts/FiraMono-Regular.cc" 
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts/FiraSans-Regular.cc")
    target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_DEFAULT_FONTS)
endif()

target_sources(glow-extras PRIVATE ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(glow-extras PUBLIC "./")
target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_TIMING)
