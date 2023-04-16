cmake_minimum_required(VERSION 3.20)

project(demo_2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(fmt REQUIRED)

set(CMAKE_BUILD_TYPE Debug)


add_executable(out
    main.cpp
)


target_link_libraries(out PUBLIC vke fmt::fmt)

target_include_directories(out PUBLIC VKE_INCLUDE)
