cmake_minimum_required(VERSION 3.20)

project(vke3)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(fmt REQUIRED)

file(GLOB LIB_CPPS "libs/*.c*")

file(GLOB VKE_SRC_FILES "${CMAKE_SOURCE_DIR}/src/vke/*.cpp")
set(VKE_INCLUDE  "${CMAKE_SOURCE_DIR}/src")

add_library(vke 

    ${VKE_SRC_FILES}
    ${LIB_CPPS}
)

#imgui manager
add_subdirectory(src/vke/imgui)

add_subdirectory(submodules/glm)


target_include_directories(vke PUBLIC ${Vulkan_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/libs/)
target_link_libraries(vke PUBLIC ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES} vke_imgui glm)

# target_compile_options(vke PRIVATE -fPIC)

target_include_directories(vke INTERFACE $<BUILD_INTERFACE:${VKE_INCLUDE}> $<INSTALL_INTERFACE:include>)

target_compile_options(vke PUBLIC
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE
    -DGLM_FORCE_RADIANS
)



# demos
add_subdirectory(demos/1)
add_subdirectory(demos/raytrace)
