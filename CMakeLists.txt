cmake_minimum_required(VERSION 3.24)
project(vke_playground)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


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

include_directories(
    src/
    submodules/glm
    submodules/entt/src
    submodules/imgui
)

# Optionally, specify additional linker options or dependencies
# target_link_libraries(${LIBRARY_NAME} <dependency>)

target_link_libraries("1.out"
    ${Vulkan_LIBRARIES} 
    vke
    ${SDL2_LIBRARIES}
)

