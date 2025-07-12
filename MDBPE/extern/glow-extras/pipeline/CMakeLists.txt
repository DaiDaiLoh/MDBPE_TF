cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE SHADER_FILES "shader/*.*")

if (GLOW_EXTRAS_EMBED_SHADERS)
    embed_files("shader/" "pipeline_embed_shaders" "${SHADER_FILES}")
endif()

file(GLOB_RECURSE SOURCE_FILES "*.cc")
file(GLOB_RECURSE HEADER_FILES "*.hh")

target_sources(glow-extras PRIVATE ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(glow-extras PUBLIC "./")
target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_PIPELINE)

if (MSVC)
    target_compile_definitions(glow-extras PUBLIC _ENABLE_EXTENDED_ALIGNED_STORAGE)
endif()

#
# Options
#

set(GLOW_EXTRAS_PIPELINE_PROFILE_CPU OFF CACHE BOOL "Perform Aion CPU profiling")

if (GLOW_EXTRAS_PIPELINE_PROFILE_CPU)
    target_compile_definitions(glow-extras PRIVATE GLOW_EXTRAS_PIPELINE_ENABLE_CPU_PROFILING)
endif()
