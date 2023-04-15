cmake_minimum_required(VERSION 3.20)

project(demo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(fmt REQUIRED)

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(vke)

add_executable(out
    src/main.cpp
)

target_link_libraries(out PUBLIC vke fmt::fmt)

# set the output directory of the executable
set_target_properties(out PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
