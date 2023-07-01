cmake_minimum_required(VERSION 3.20)

project(vke)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_PREFIX_PATH "/usr/lib" ${CMAKE_PREFIX_PATH})


find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(fmt REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SHADERC REQUIRED shaderc)



file(GLOB LIB_CPPS "libs/*.c*")

file(GLOB_RECURSE VKE_SRC_FILES "src/vke/*.cpp")

set(VKE_INCLUDE "src")

add_library(vke 

    ${VKE_SRC_FILES}
    ${LIB_CPPS}
)

#imgui manager
add_subdirectory(src/vke/imgui)

add_subdirectory(submodules/glm)


target_link_libraries(vke PUBLIC ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES} vke_imgui glm ${SHADERC_LIBRARIES})

# target_compile_options(vke PRIVATE -fPIC)

target_include_directories(vke PUBLIC ${Vulkan_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/libs/ ${SHADERC_INCLUDE_DIRS})
target_include_directories(vke INTERFACE ${VKE_INCLUDE})

target_compile_options(vke PUBLIC
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE
    -DGLM_FORCE_RADIANS
)



# demos
# add_subdirectory(demos/1)
# add_subdirectory(demos/2)
# add_subdirectory(demos/raytrace)
