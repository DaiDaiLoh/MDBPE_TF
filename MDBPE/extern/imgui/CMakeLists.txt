cmake_minimum_required(VERSION 3.0)
project(ImGui)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(IMGUI_USE_IMPLOT "add implot" ON)

# auto ogl
if (TARGET glow)
    set(IMGUI_SUPPORT_OPENGL TRUE)
endif()

# auto vulkan
if (TARGET lava)
    set(IMGUI_SUPPORT_VULKAN TRUE)
endif()

# auto glfw
if (TARGET glfw)
    set(IMGUI_SUPPORT_GLFW TRUE)
endif()

# =====================================================
# setup sources

set(SOURCES
    "src/imgui/imconfig.h"
    "src/imgui/imgui.cpp"
    "src/imgui/imgui.h"
    "src/imgui/imguizmo.cpp"
    "src/imgui/imguizmo.h"
    "src/imgui/imgui_internal.h"
    "src/imgui/imgui_draw.cpp"
    "src/imgui/imgui_demo.cpp"
    "src/imgui/imgui_widgets.cpp"
    "src/imgui/imgui_stdlib.cpp"
    "src/imgui/imgui_stdlib.h"
    "src/imgui/imgui_widgets.cpp"
    "src/imgui/imgui_tables.cpp"
    "src/imgui/imstb_rectpack.h"
    "src/imgui/imstb_textedit.h"
    "src/imgui/imstb_truetype.h"
)

if (IMGUI_SUPPORT_OPENGL)
    list(APPEND SOURCES
        "src/imgui/imgui_impl_opengl3.h"
        "src/imgui/imgui_impl_opengl3.cpp"
        "src/imgui/imgui_impl_opengl3_loader.h"
    )
endif()

if (IMGUI_SUPPORT_VULKAN)
    list(APPEND SOURCES
        "src/imgui/imgui_impl_vulkan.h"
        "src/imgui/imgui_impl_vulkan.cpp"
    )
endif()

if (IMGUI_SUPPORT_GLFW)
    list(APPEND SOURCES
        "src/imgui/imgui_impl_glfw.h"
        "src/imgui/imgui_impl_glfw.cpp"
    )
endif()

if (IMGUI_USE_IMPLOT)
    list(APPEND SOURCES
        "src/imgui/implot.h"
        "src/imgui/implot_internal.h"
        "src/imgui/implot.cpp"
        "src/imgui/implot_items.cpp"
    )
endif()


# =====================================================
# create library
add_library(imgui STATIC ${SOURCES})
target_include_directories(imgui PUBLIC src)

if (IMGUI_SUPPORT_GLFW)
    target_link_libraries(imgui PRIVATE glfw)
endif()

if (IMGUI_SUPPORT_VULKAN)
    target_link_libraries(imgui PUBLIC volk)
endif()

if (IMGUI_SUPPORT_OPENGL)
    target_link_libraries(imgui PRIVATE glad)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
endif()

if (IMGUI_USE_IMPLOT)
    target_compile_definitions(imgui PUBLIC IMGUI_USE_IMPLOT)
endif()
