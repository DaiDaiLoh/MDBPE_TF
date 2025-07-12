
cmake_minimum_required(VERSION 3.8)
project(tokenprocessor)

# ===============================================
# global settings

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ===============================================
# options

option(TOK_ENABLE_ASAN "if true, enables clang/MSVC address sanitizer" OFF)
option(TOK_ENABLE_MSAN "if true, enables clang/MSVC memory sanitizer" OFF)
option(TOK_ENABLE_UBSAN "if true, enables clang/MSVC undefined behaviour sanitizer" OFF)
option(TOK_ENABLE_TSAN "if true, enables clang/MSVC thread sanitizer" OFF)

if (TOK_ENABLE_ASAN AND TOK_ENABLE_TSAN)
    message(FATAL_ERROR "Can only enable one of TSan or ASan at a time")
endif()
if (TOK_ENABLE_ASAN AND TOK_ENABLE_MSAN)
    message(FATAL_ERROR "Can only enable one of ASan or MSan at a time")
endif()

option(TOK_ENABLE_WERROR "if true, enables -Werror, /WX" OFF)


# ===============================================
# compiler and linker flags

set(COMMON_COMPILER_FLAGS "")
set(COMMON_LINKER_FLAGS "")

if (MSVC)
    list(APPEND COMMON_COMPILER_FLAGS /MP)

    if (TOK_ENABLE_WERROR)
        list(APPEND COMMON_COMPILER_FLAGS /WX)
    endif()
else()
    list(APPEND COMMON_COMPILER_FLAGS -Wall -Wextra)

    if (TOK_ENABLE_WERROR)
        list(APPEND COMMON_COMPILER_FLAGS -Werror)
    endif()

    if (TOK_ENABLE_ASAN OR TOK_ENABLE_TSAN OR TOK_ENABLE_MSAN OR TOK_ENABLE_UBSAN)
        list(APPEND COMMON_COMPILER_FLAGS -fno-omit-frame-pointer -g)
        list(APPEND COMMON_LINKER_FLAGS -fno-omit-frame-pointer -g)
    endif()

    if (TOK_ENABLE_ASAN)
        list(APPEND COMMON_COMPILER_FLAGS -fsanitize=address)
        list(APPEND COMMON_LINKER_FLAGS -fsanitize=address)
    endif()

    if (TOK_ENABLE_TSAN)
        list(APPEND COMMON_COMPILER_FLAGS -fsanitize=thread)
        list(APPEND COMMON_LINKER_FLAGS -fsanitize=thread)
    endif()

    if (TOK_ENABLE_MSAN)
        list(APPEND COMMON_COMPILER_FLAGS -fsanitize=memory)
        list(APPEND COMMON_LINKER_FLAGS -fsanitize=memory)
    endif()

    if (TOK_ENABLE_UBSAN)
        list(APPEND COMMON_COMPILER_FLAGS
            -fsanitize=undefined
            -fno-sanitize-recover=all
            -fno-sanitize=alignment,vptr
        )
        list(APPEND COMMON_LINKER_FLAGS
            -fsanitize=undefined
            -fno-sanitize-recover=all
            -fno-sanitize=alignment,vptr
        )
    endif()
endif()

# ===============================================
# Bin dir
if(MSVC)
    set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin)
elseif(CMAKE_BUILD_TYPE STREQUAL "")
    set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin/Default)
else()
    set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR})
set(GLOW_BIN_DIR ${CMAKE_SOURCE_DIR}/bin)

# ===============================================
# disable glfw additionals
option(GLFW_BUILD_EXAMPLES  OFF)
option(GLFW_BUILD_TESTS  OFF)
option(GLFW_BUILD_DOCS  OFF)
option(GLFW_INSTALL  OFF)


# ===============================================s
# add submodules
add_subdirectory(extern/clean-core)
add_subdirectory(extern/typed-geometry)
add_subdirectory(extern/ctracer)
add_subdirectory(extern/reflector)
add_subdirectory(extern/rich-log)
add_subdirectory(extern/glfw)
add_subdirectory(extern/glow)
add_subdirectory(extern/imgui)
add_subdirectory(extern/glow-extras)
add_subdirectory(extern/nexus)
add_subdirectory(extern/babel-serializer)
add_subdirectory(extern/clean-ranges)
add_subdirectory(extern/image)
add_subdirectory(extern/cpp-utils/)

find_package(OpenMP REQUIRED)

# ===============================================
# configure executable
file(GLOB_RECURSE SOURCES
    "src/*.cc"
    "src/*.hh"
    "src/*.cpp"
    "src/*.h"
)

# group sources according to folder structure
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# ===============================================
# add executable
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC
    clean-core
    typed-geometry
    ctracer
    reflector
    rich-log
    glfw
    glow
    imgui
    glow-extras
    nexus
    babel-serializer
    clean-ranges
    image
    cpp-utils

    OpenMP::OpenMP_CXX

    ${COMMON_LINKER_FLAGS}
)

target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_compile_options(${PROJECT_NAME} PUBLIC ${COMMON_COMPILER_FLAGS})

# dependency grouping in the IDE
foreach(TARGET_NAME
    clean-core
    typed-geometry
    ctracer
    reflector
    rich-log
    glfw
    glow
    imgui
    glow-extras
    nexus
    babel-serializer
    clean-ranges
    image
    cpp-utils

)
    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "Extern")
endforeach()

