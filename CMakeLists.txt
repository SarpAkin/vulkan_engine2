cmake_minimum_required(VERSION 3.24)
project(vke_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Set the library name and source files
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/submodules/imgui")
list(APPEND SRC_FILES 
"${IMGUI_DIR}/imgui.h"
"${IMGUI_DIR}/imgui.cpp"
"${IMGUI_DIR}/imgui_demo.cpp"
"${IMGUI_DIR}/imgui_tables.cpp"
"${IMGUI_DIR}/imgui_draw.cpp"
"${IMGUI_DIR}/imgui_widgets.cpp"
"${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
"${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
)


set(VKE_ENGINE_INCLUDE "include")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(Vulkan REQUIRED)

add_library("vke_engine" ${SRC_FILES})

add_subdirectory(submodules/vke_core)
add_subdirectory(submodules/flecs)

target_link_libraries(vke_engine PUBLIC
    ${Vulkan_LIBRARIES} 
    vke
    flecs
    ${SDL2_LIBRARIES}
)


target_include_directories(vke_engine PUBLIC 
    submodules/glm
    submodules/flecs/include
    submodules/imgui
    include/
    src/
)


if(CMAKE_BUILD_TYPE STREQUAL "DebugOnlyEngine")
    target_compile_options(vke_engine PRIVATE -O0 -g)

    set_target_properties(vke PROPERTIES
        COMPILE_FLAGS "-O3 -g"
    )
endif()