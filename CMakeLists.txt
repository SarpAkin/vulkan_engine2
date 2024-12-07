cmake_minimum_required(VERSION 3.24)
project(vke_playground)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


# Set the library name and source files
file(GLOB_RECURSE SRC_FILES "src/*.cpp")


find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(Vulkan REQUIRED)

# Create the shared library
add_executable("1.out" ${SRC_FILES})

add_subdirectory(submodules/vke_core)

include_directories(
    src/
    submodules/glm
)

# Optionally, specify additional linker options or dependencies
# target_link_libraries(${LIBRARY_NAME} <dependency>)

target_link_libraries("1.out"
    ${Vulkan_LIBRARIES} 
    vke
    ${SDL2_LIBRARIES}
)

