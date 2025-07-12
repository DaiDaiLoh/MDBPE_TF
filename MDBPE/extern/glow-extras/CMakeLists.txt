cmake_minimum_required(VERSION 3.0)
project(glow-extras)

#
# Options
#

set(GLOW_EXTRAS_EMBED_SHADERS ON CACHE BOOL "If true, shaders are embedded into the application")

set(GLOW_EXTRAS_OPENGL_DEBUG_GROUPS ON CACHE BOOL "Set OpenGL debug groups for better legibility in RenderDoc etc.")

set(GLOW_EXTRAS_DEFAULT_FONTS ON CACHE BOOL "If true, adds 'sans' and 'mono' (Fira) default fonts to the binary dir and loads them into the viewer")

#
# Utility
#

set(GLOW_EXTRAS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(generate_embedded_files fileDir files outputPath outputName)
    set(embedDir "${CMAKE_CURRENT_LIST_DIR}/${fileDir}")
    set(outputCC "${outputPath}.cc")
    set(outputHH "${outputPath}.hh")

    set(existingFilesObsolete TRUE)
    set(existingFilesPresent FALSE)
    set(existingFilePath "")

    # Check if generated files already exist
    if (EXISTS ${outputCC})
        set(existingFilesObsolete FALSE)
        set(existingFilesPresent TRUE)
        set(existingFilePath ${outputCC})
    endif()

    set(resourceCount 0)
    set(registrationCode "")
    foreach(bin ${files})

        # If the existing files are not considered obsolete yet,
        # check if the current resource is newer than the generated file
        if (${existingFilesPresent} AND NOT ${existingFilesObsolete})
            if (${bin} IS_NEWER_THAN ${existingFilePath})
                set(existingFilesObsolete TRUE)
            endif()
        endif()

        # ++resourceCount
        MATH(EXPR resourceCount "${resourceCount}+1")

        # Read the resource from disk and append its contents to registrationCode
        string(REPLACE "${embedDir}" "" relativeBinPath "${bin}")
        file(READ ${bin} filedataString)
        set(registrationCode "${registrationCode}{\"${relativeBinPath}\", R\"%%RES_EMBED%%(\n${filedataString}\n)%%RES_EMBED%%\"},\n")
    endforeach()

    # Only write files if > 0 resources exist and if the existing files are obsolete
    if(${resourceCount} AND ${existingFilesObsolete})
        # Write the .hh file
        file(WRITE ${outputHH} "#pragma once\n\n#include <utility>\n\n")
        file(APPEND ${outputHH} "// This file is generated upon running CMake, do not modify it!\n\n")
        file(APPEND ${outputHH} "namespace internal_embedded_files {\n")
        file(APPEND ${outputHH} "extern const std::pair<const char*, const char*> ${outputName}[${resourceCount}];\n}\n")

        # Retrieve .hh filename from path ("path.split('/').back()")
        # This is not necessarily equal to outputName
        string(REPLACE "/" ";" outputPathList ${outputHH})
        list(LENGTH outputPathList outputPathLength)
        MATH(EXPR outputHeaderFilenameIndex "${outputPathLength}-1")
        list(GET outputPathList ${outputHeaderFilenameIndex} outputHeaderFilename)

        # Write the .cc file
        file(WRITE ${outputCC} "#include \"${outputHeaderFilename}\"\n\n")
        file(APPEND ${outputCC} "// This file is generated upon running CMake, do not modify it!\n\n")
        file(APPEND ${outputCC} "namespace internal_embedded_files {\n")
        file(APPEND ${outputCC} "\nconst std::pair<const char*, const char*> ${outputName}[] = {\n${registrationCode}\n};\n}\n")
    endif()
endfunction()

function(embed_files fileDir outputName files)
    # Simple wrapper for generate_embedded_files, internal use only
    generate_embedded_files("${fileDir}" "${files}" "${GLOW_EXTRAS_ROOT_DIR}/generated/glow-extras/generated/${outputName}" "${outputName}")
endfunction()

if (NOT GLOW_BIN_DIR)
    message(FATAL_ERROR "[glow-extras] GLOW_BIN_DIR not set (is glow not up-to-date? or in the wrong directory?)")
endif()

# ===================================================================================================
# Initialize glow-extras library

# all glow-extras
add_library(glow-extras ${GLOW_LINK_TYPE})

if (NOT TARGET clean-core)
    message(FATAL_ERROR "[glow-extras] requires clean-core")
endif()
target_link_libraries(glow-extras PUBLIC glow clean-core)

if (MSVC)
    target_compile_options(glow-extras PUBLIC /MP /FC)
else()
    target_compile_options(glow-extras PRIVATE -Wall)
endif()

if (GLOW_EXTRAS_EMBED_SHADERS)
    target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_EMBED_SHADERS)
endif()

if(GLOW_EXTRAS_OPENGL_DEBUG_GROUPS)
    target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_OPENGL_DEBUG_GROUPS)
endif()

# ===================================================================================================
# Check available targets

if (TARGET AntTweakBar)
    target_link_libraries(glow-extras PUBLIC AntTweakBar)
    target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_HAS_ANTTWEAKBAR)
endif()

if (TARGET imgui)
    target_link_libraries(glow-extras PUBLIC imgui)
    target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_HAS_IMGUI)
endif()

if (TARGET aion)
    target_link_libraries(glow-extras PUBLIC aion)
    target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_HAS_AION)
endif()

if (TARGET assimp)
    target_link_libraries(glow-extras PUBLIC assimp)
    target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_HAS_ASSIMP)
endif()

if (TARGET polymesh)
    target_link_libraries(glow-extras PUBLIC polymesh)
    target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_HAS_POLYMESH)
endif()

if (TARGET glfw)
    target_link_libraries(glow-extras PUBLIC glfw)
    target_compile_definitions(glow-extras PUBLIC GLOW_EXTRAS_HAS_GLFW)
endif()


# ===================================================================================================
# Add components

# Input
add_subdirectory(input)

# Colors
add_subdirectory(colors)

# Camera
add_subdirectory(camera)

# Shader
add_subdirectory(shader)

# Geometry
add_subdirectory(geometry)

# Pipeline
add_subdirectory(pipeline)

# Debug
add_subdirectory(debugging)

# Material shading library
add_subdirectory(material)

# Timing
add_subdirectory(timing)

# Vector
add_subdirectory(vector)

# Assimp
# requires 'assimp'
if (TARGET assimp)
    add_subdirectory(assimp)
else()
    message(STATUS "[glow-extras] target 'assimp' not found, disabling glow-extras-assimp")
endif()

# GLFW App
add_subdirectory(glfw)

# Viewer
# requires 'polymesh'
if (TARGET polymesh)
    add_subdirectory(viewer)
else()
    message(STATUS "[glow-extras] target 'polymesh' not found, disabling glow-extras-viewer")
endif()

# Generated files
# this must be the last subdirectory
if (GLOW_EXTRAS_EMBED_SHADERS)
    add_subdirectory(generated)
endif()

# ===================================================================================================
# Folders

if(${CMAKE_VERSION} VERSION_GREATER "3.8.0")
    get_target_property(GLOW_EXTRAS_SOURCES glow-extras SOURCES)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GLOW_EXTRAS_SOURCES})
endif()
