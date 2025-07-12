cmake_minimum_required(VERSION 3.0)
project(glow)

# ==================================
# Options

# minimum opengl
# e.g. CORE_42 or COMPATIBILITY_33
set(GLOW_OPENGL_SUPPORT CORE_45 CACHE STRING "Defines the OpenGL version with supported C++ code (higher versions work, but GLOW features are disabled. If you have a lower version, GLOW may crash when using those)")

# libs
set(GLOW_USE_OWN_GLAD ON CACHE BOOL "If true, extern/glad is used")

# build options
set(GLOW_LINK_TYPE SHARED CACHE STRING "Defines the build type of libraries (shared is default)")
set(GLOW_USE_GOLD_LINKER ON CACHE BOOL "If true, ld.gold is used for linking")
set(GLOW_ENABLE_MARCH_NATIVE ON CACHE BOOL "If true, adds -march=native")

# misc
set(GLOW_AUTO_PROFILING ON CACHE BOOL "If true and aion target is present, glow will profile some likely-to-be-costly functions")
set(GLOW_VALIDATION ON CACHE BOOL "If true, checks if glow is initialized and called from the correct thread (very low performance impact)")
set(GLOW_CHECK_CXX_STANDARD ON CACHE BOOL "If true, checks if C++ standard is set globally (this is strongly recommended)")
set(GLOW_EXPERIMENTAL_THREAD_AGNOSTIC OFF CACHE BOOL "If true, glow does not use thread local and leaves thread management to the user")
set(GLOW_CMAKE_REPORT OFF CACHE BOOL "If true, prints a more elaborate report in cmake")

# binary dir
if (NOT GLOW_BIN_DIR)
    set(GLOW_BIN_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(GLOW_BIN_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} PARENT_SCOPE)
    message(STATUS "[glow] set GLOW_BIN_DIR to ${GLOW_BIN_DIR}")
endif()

# ==================================
# Basics
include(cmake/basic-setup.cmake)
include(cmake/UnityBuild.cmake)
if (GLOW_CMAKE_REPORT)
    message(STATUS "GLOW CMake Config")
    message(STATUS "  Operating System : ${OPERATING_SYSTEM}")
    message(STATUS "  Compiler         : ${COMPILER}")
    message(STATUS "  OpenGL           : ${GLOW_OPENGL_SUPPORT}")
endif()

if(MSVC OR APPLE)
    # we need to link statically on Windows and OSX
    set(GLOW_LINK_TYPE STATIC CACHE STRING "" FORCE)
endif()

if(APPLE)
    set(GLOW_USE_GOLD_LINKER OFF CACHE STRING "" FORCE) # no gold linker on mac
endif()

if (GLOW_CHECK_CXX_STANDARD)
    if (NOT "${CMAKE_CXX_STANDARD}" GREATER 16)
        message(FATAL_ERROR "[glow] CMAKE_CXX_STANDARD is less than 17, please specify at least SET(CMAKE_CXX_STANDARD 17)")
    endif()
endif()

option(GLOW_ENABLE_UNITY_BUILD "If enabled, compiles this library as a single compilation unit" ON)

# ==================================
# Collect files
file(GLOB_RECURSE SOURCE_FILES "src/*.cc")
file(GLOB_RECURSE HEADER_FILES "src/*.hh" "src/*.h")
file(GLOB_RECURSE TEST_FILES "test/*.cc" "test/*.hh")
file(GLOB_RECURSE SCRIPT_FILES "scripts/*.sh" "scripts/*.py")
if(${CMAKE_VERSION} VERSION_GREATER "3.8.0")
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SOURCE_FILES} ${HEADER_FILES})
endif()

if (GLOW_ENABLE_UNITY_BUILD)
    arcana_enable_unity_build(glow SOURCE_FILES 100 cc)
endif()

# ==================================
# Create GLOW target
add_library(glow ${GLOW_LINK_TYPE}
    ${SOURCE_FILES}
    ${HEADER_FILES}
    )

# ==================================
# Compiler flags
if(NOT MSVC)
    target_compile_options(glow PRIVATE
        # for extra errors
        -Wextra
        # -Wpedantic # glm broke this :(
        # -Wshadow # glm has a lot of these unfortunately
        # -Wstrict-overflow # false positives!
        -Wno-unused-parameter # we don't consider this an error
        )
    if(GCC)
        target_compile_options(glow PRIVATE -Wdouble-promotion)
    else()
        # clang supports this only at newer version
    endif()
    if (CLANG)
        target_compile_options(glow PUBLIC
            # -Wno-strict-aliasing # Temporary fix for https://github.com/g-truc/glm/issues/473
            -Wno-gnu-anonymous-struct # used by glm
            -Wno-nested-anon-types    # used by glm
            )
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.0)
            target_compile_options(glow PUBLIC
                -Wno-undefined-var-template # glTypeOf<>
                )
        endif()
    endif()

    target_link_libraries(glow PUBLIC pthread)

    # gold linker
    if(GLOW_USE_GOLD_LINKER)
        # WRONG: target_compile_options(glow PUBLIC -fuse-ld=gold )
        target_link_libraries(glow PUBLIC  -fuse-ld=gold)
    endif()

    # arch-dependend optimization
    if(GLOW_ENABLE_MARCH_NATIVE)
        # optimize for native arch
        target_compile_options(glow PUBLIC -march=native)
    else()
        # otherwise at least sse4.2
        target_compile_options(glow PUBLIC -msse4.2)
    endif()
else() # MSVC
    target_compile_options(glow PUBLIC
        /MP # multi processor compilation
        /FC # proper absolute paths from __FILE__
        )
    target_compile_definitions(glow PUBLIC
        NOMINMAX            # windows.h ...
        WIN32_LEAN_AND_MEAN # windows.h ...
        )
endif()

# ==================================
# Defines
target_compile_definitions(glow PUBLIC
    # OS
    GLOW_OS_${OPERATING_SYSTEM}

    # Compiler
    GLOW_COMPILER_${COMPILER}

    # OpenGL
    GLOW_OPENGL_SUPPORT_${GLOW_OPENGL_SUPPORT}
    )

# other OpenGL defines
if(${GLOW_OPENGL_SUPPORT} MATCHES "CORE_")
    set(GLOW_OPENGL_PROFILE CORE)
else()
    set(GLOW_OPENGL_PROFILE COMPATIBILITY)
endif()
target_compile_definitions(glow PUBLIC GLOW_OPENGL_PROFILE_${GLOW_OPENGL_PROFILE})
target_compile_definitions(glow PUBLIC GLOW_OPENGL_PROFILE=${GLOW_OPENGL_PROFILE})
string(REGEX MATCH "([0-9]+)" GLOW_OPENGL_VERSION ${GLOW_OPENGL_SUPPORT})
target_compile_definitions(glow PUBLIC GLOW_OPENGL_VERSION_${GLOW_OPENGL_VERSION})
target_compile_definitions(glow PUBLIC GLOW_OPENGL_VERSION=${GLOW_OPENGL_VERSION})

if(${GLOW_VALIDATION})
    target_compile_definitions(glow PUBLIC GLOW_PERFORM_VALIDATION)
endif()

if (${GLOW_EXPERIMENTAL_THREAD_AGNOSTIC})
    target_compile_definitions(glow PUBLIC GLOW_THREAD_AGNOSTIC)
endif()

if(GCC)
    # workaround for https://github.com/nothings/stb/issues/280
    target_link_libraries(glow PUBLIC gcc_s gcc)
endif()

# ==================================
# Include Dirs
target_include_directories(glow PUBLIC
    src/
    )

# ==================================
# Libraries

# OpenGL
set(OpenGL_GL_PREFERENCE GLVND) # prefer new opengl
find_package(OpenGL REQUIRED)
target_link_libraries(glow PUBLIC ${OpenGL_LIBRARIES})
if(WIN32) # Windows
    target_link_libraries(glow PUBLIC Opengl32)
elseif(APPLE) # Probably OSX
    find_library(OPENGL_FRAMEWORK OpenGL)
    target_link_libraries(glow PUBLIC ${OPENGL_FRAMEWORK})
else() # Probably Linux
    target_link_libraries(glow PUBLIC GL)
endif()

# Qt if found
if(Qt5Gui_FOUND)
    message(STATUS "[glow] found Qt GUI, building with Qt support.")
    target_link_libraries(glow PUBLIC ${Qt5Gui_LIBRARIES})
    target_compile_definitions(glow PUBLIC GLOW_USE_QT)
endif()

# GLM
if(TARGET glm)
    target_link_libraries(glow PUBLIC glm)
    target_compile_definitions(glow PUBLIC GLOW_USE_GLM)
    target_compile_definitions(glow PUBLIC GLOW_HAS_GLM)
endif()

# clean-core
if (NOT TARGET clean-core)
    message(FATAL_ERROR "[glow] requires clean-core")
endif()
target_link_libraries(glow PUBLIC clean-core)

# typed geometry
if(TARGET typed-geometry)
    target_link_libraries(glow PUBLIC typed-geometry)
    target_compile_definitions(glow PUBLIC GLOW_HAS_TG)
else()
    message(FATAL_ERROR "no target 'typed-geomtry' found. GLOW requires typed-geometry. Is a submodule/dependency missing?")
endif()

# GLAD loader
if(GLOW_USE_OWN_GLAD)
    set(GLAD_VERSION ${GLOW_OPENGL_VERSION})
    string(REGEX REPLACE "(.)(.)" "\\1.\\2" GLAD_VERSION ${GLAD_VERSION})
    set(GLAD_PROFILE ${GLOW_OPENGL_PROFILE})
    add_subdirectory(extern/glad)
    target_link_libraries(glow PUBLIC glad)
endif()

# AION
if(GLOW_AUTO_PROFILING)
    if (TARGET aion)
        message(STATUS "[glow] building with aion-profiling ('aion' target found and auto-profiling active)")
        target_link_libraries(glow PUBLIC aion)
        target_compile_definitions(glow PUBLIC GLOW_AION_PROFILING)
    endif()
endif()

# ==================================
# Report
if(GLOW_CMAKE_REPORT)
    message(STATUS "")
    message(STATUS "**************************************")
    message(STATUS "GLOW CMake Report")
    message(STATUS "")
    message(STATUS "  Linked Libraries:")
    get_target_property(GLOW_LIBS glow INTERFACE_LINK_LIBRARIES)
    foreach(v ${GLOW_LIBS})
        message(STATUS "    * ${v}")
    endforeach()
    message(STATUS "")
    message(STATUS "  Defines:")
    get_target_property(GLOW_DEFINES glow INTERFACE_COMPILE_DEFINITIONS)
    foreach(v ${GLOW_DEFINES})
        message(STATUS "    * ${v}")
    endforeach()
    message(STATUS "")
    message(STATUS "  Compile Options:")
    get_target_property(GLOW_COMPILE_OPTIONS glow INTERFACE_COMPILE_OPTIONS)
    foreach(v ${GLOW_COMPILE_OPTIONS})
        message(STATUS "    * ${v}")
    endforeach()
    message(STATUS "")
    message(STATUS "  Include directories:")
    get_target_property(GLOW_INCLUDE_DIRECTORIES glow INTERFACE_INCLUDE_DIRECTORIES)
    foreach(v ${GLOW_INCLUDE_DIRECTORIES})
        message(STATUS "    * ${v}")
    endforeach()
    message(STATUS "")
    message(STATUS "**************************************")
    message(STATUS "")
endif()
