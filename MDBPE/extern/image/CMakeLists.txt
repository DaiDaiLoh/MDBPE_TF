cmake_minimum_required(VERSION 3.8)

project(
    image
    VERSION 0.1
    DESCRIPTION "A simple to use image library"
    LANGUAGES CXX
)

# ===============================================
# global settings

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ===============================================
# Dependencies

if(NOT TARGET clean-core)
    message(FATAL_ERROR "[image] No target 'clean-core' found. image requires clean-core. Is a submodule/dependency missing?")
endif()

if(NOT TARGET typed-geometry)
    message(FATAL_ERROR "[image] No target 'typed-geometry' found. image requires typed-geometry. Is a submodule/dependency missing?")
endif()

if(NOT TARGET babel-serializer)
    message(FATAL_ERROR "[image] No target 'babel-serializer' found. image requires babel-serializer. Is a submodule/dependency missing?")
endif()

# ===============================================
# options

option(IMAGE_ENABLE_ASAN "if true, enables clang/MSVC address sanitizer" OFF)
option(IMAGE_ENABLE_MSAN "if true, enables clang/MSVC memory sanitizer" OFF)
option(IMAGE_ENABLE_UBSAN "if true, enables clang/MSVC undefined behaviour sanitizer" OFF)
option(IMAGE_ENABLE_TSAN "if true, enables clang/MSVC thread sanitizer" OFF)

if (IMAGE_ENABLE_ASAN AND IMAGE_ENABLE_TSAN)
    message(FATAL_ERROR "Can only enable one of TSan or ASan at a time")
endif()
if (IMAGE_ENABLE_ASAN AND IMAGE_ENABLE_MSAN)
    message(FATAL_ERROR "Can only enable one of ASan or MSan at a time")
endif()

option(IMAGE_ENABLE_WERROR "if true, enables -Werror, /WX" OFF)


# ===============================================
# compiler and linker flags

set(COMMON_COMPILER_FLAGS "")
set(COMMON_LINKER_FLAGS "")

if (MSVC)
    list(APPEND COMMON_COMPILER_FLAGS /MP)

    if (IMAGE_ENABLE_WERROR)
        list(APPEND COMMON_COMPILER_FLAGS /WX)
    endif()
else()
    list(APPEND COMMON_COMPILER_FLAGS -Wall -Wextra)

    if (IMAGE_ENABLE_WERROR)
        list(APPEND COMMON_COMPILER_FLAGS -Werror)
    endif()

    if (IMAGE_ENABLE_ASAN OR IMAGE_ENABLE_TSAN OR IMAGE_ENABLE_MSAN OR IMAGE_ENABLE_UBSAN)
        list(APPEND COMMON_COMPILER_FLAGS -fno-omit-frame-pointer -g)
        list(APPEND COMMON_LINKER_FLAGS -fno-omit-frame-pointer -g)
    endif()

    if (IMAGE_ENABLE_ASAN)
        list(APPEND COMMON_COMPILER_FLAGS -fsanitize=address)
        list(APPEND COMMON_LINKER_FLAGS -fsanitize=address)
    endif()

    if (IMAGE_ENABLE_TSAN)
        list(APPEND COMMON_COMPILER_FLAGS -fsanitize=thread)
        list(APPEND COMMON_LINKER_FLAGS -fsanitize=thread)
    endif()

    if (IMAGE_ENABLE_MSAN)
        list(APPEND COMMON_COMPILER_FLAGS -fsanitize=memory)
        list(APPEND COMMON_LINKER_FLAGS -fsanitize=memory)
    endif()

    if (IMAGE_ENABLE_UBSAN)
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
# library

file(GLOB_RECURSE SOURCE_FILES "src/*.cc")
file(GLOB_RECURSE HEADER_FILES "src/*.hh")

# group sources according to folder structure
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES} ${HEADER_FILES})

# ===============================================
# add executable
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC src/)

target_link_libraries(${PROJECT_NAME} PUBLIC
    clean-core
    typed-geometry
    babel-serializer
)

if(TARGET fftw-cpp)
    message("-- [image] Found 'fftw-cpp'. Enabling fftw features.")
    target_compile_definitions(${PROJECT_NAME} PUBLIC IMG_HAS_FFTW_CPP)
    target_link_libraries(${PROJECT_NAME} PUBLIC fftw-cpp)
endif()
