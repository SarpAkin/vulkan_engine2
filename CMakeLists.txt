cmake_minimum_required(VERSION 3.30)
project(vke_playground)

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


find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(Vulkan REQUIRED)

# Create the shared library
add_executable("1.out" ${SRC_FILES})

add_subdirectory(submodules/vke_core)
add_subdirectory(submodules/flecs)

include_directories(
    submodules/glm
    submodules/entt/src
    submodules/flecs/include
    submodules/imgui
    PRIVATE
    src/
    pch/
)

# Optionally, specify additional linker options or dependencies
# target_link_libraries(${LIBRARY_NAME} <dependency>)

target_link_libraries("1.out"
    ${Vulkan_LIBRARIES} 
    vke
    flecs
    ${SDL2_LIBRARIES}
)

# Precompiled headers
target_precompile_headers("1.out" PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:pch.hpp>")

# if(CMAKE_BUILD_TYPE STREQUAL "DebugOnlyEngine")
#     target_compile_options("1.out" PRIVATE -O0 -g)

#     set_target_properties(vke PROPERTIES
#         COMPILE_FLAGS "-O3 -g"
#     )
# endif()