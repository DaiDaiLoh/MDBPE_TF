cmake_minimum_required(VERSION 3.1)

project(cpp-utils 
    VERSION 0.0.1
    DESCRIPTION "A collection of small cpp utilities"
    LANGUAGES CXX
)

if (NOT TARGET typed-geometry)
    message(FATAL_ERROR "no target 'typed-geomtry' found. cpp-utils requires typed-geometry. Is a submodule/dependency missing?")
endif()

if (NOT TARGET clean-core)
    message(FATAL_ERROR "no target 'clean-core' found. cpp-utils requires typed-geometry. Is a submodule/dependency missing?")
endif()

if (NOT TARGET rich-log)
    message(FATAL_ERROR "no target 'rich-log' found. cpp-utils requires typed-geometry. Is a submodule/dependency missing?")
endif()


file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/*.hh")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cc")

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src)

target_link_libraries(${PROJECT_NAME} PUBLIC
    clean-core
    typed-geometry
    rich-log
)

