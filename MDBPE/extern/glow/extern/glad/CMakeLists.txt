cmake_minimum_required(VERSION 3.0)
project(glad)

# Usage:
# set(GLAD_VERSION 4.4)
# set(GLAD_PROFILE CORE)

if (NOT GLAD_VERSION)
    message(ERROR "No GLAD_VERSION set")
endif()
if (NOT GLAD_PROFILE)
    message(ERROR "No GLAD_PROFILE set")
endif()

set(GLAD_PATH "${GLAD_PROFILE}-${GLAD_VERSION}")
string(TOLOWER ${GLAD_PATH} GLAD_PATH)

if (GLOW_CMAKE_REPORT)
    message(STATUS "including glad ${GLAD_PATH}")
endif()

add_library(glad ${GLOW_LINK_TYPE}
    ${GLAD_PATH}/src/glad.c
    ${GLAD_PATH}/include/glad/glad.h
    ${GLAD_PATH}/include/KHR/khrplatform.h
    )
target_include_directories(glad PUBLIC ${GLAD_PATH}/include)

if (NOT MSVC)
    target_link_libraries(glad PRIVATE dl)
else()
    target_link_libraries(glad PRIVATE legacy_stdio_definitions)
endif()
