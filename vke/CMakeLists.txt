cmake_minimum_required(VERSION 3.20)

project(vke3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(fmt REQUIRED)

file(GLOB LIB_CPPS "../libs/*.cpp")

file(GLOB VKE_SRC_FILES "*.cpp")
file(GLOB VKE_INCLUDE "*.hpp")

add_library(vke 

    ${VKE_SRC_FILES}
    ${LIB_CPPS}
)

target_include_directories(vke PUBLIC ${Vulkan_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../libs/)
target_link_libraries(vke PUBLIC ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES})
# target_compile_options(vke PRIVATE -fPIC)