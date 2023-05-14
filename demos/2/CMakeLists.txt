cmake_minimum_required(VERSION 3.20)

project(demo_2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(fmt REQUIRED)

set(CMAKE_BUILD_TYPE Debug)

find_program(GLSLC_EXECUTABLE NAMES glslc PATHS /usr/bin /usr/local/bin)

set(SHADER_DIR "res")
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/res/spirv")

file(GLOB SHADER_FILES
    "${SHADER_DIR}/*.vert"
    "${SHADER_DIR}/*.frag"
    "${SHADER_DIR}/*.comp"
)

foreach(shader_file IN LISTS SHADER_FILES)
    get_filename_component(shader_name ${shader_file} NAME)
    set(binary_file "${SHADER_BINARY_DIR}/${shader_name}.spv")
    add_custom_command(
        OUTPUT ${binary_file}
        COMMAND ${GLSLC_EXECUTABLE} -o ${binary_file} ${shader_file}
        DEPENDS ${shader_file}
    )
    list(APPEND shader_binaries ${binary_file})
endforeach()


add_executable(demo_2.out
    main.cpp
    ${shader_binaries}
)

target_link_libraries(demo_2.out PUBLIC vke fmt::fmt)

target_include_directories(demo_2.out PUBLIC VKE_INCLUDE "../../submodules/imgui")
