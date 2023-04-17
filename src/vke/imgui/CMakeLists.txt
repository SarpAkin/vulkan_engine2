

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../submodules/imgui/")

add_library(vke_imgui 
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_manager.cpp"

    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
)


target_include_directories(vke_imgui PUBLIC ${Vulkan_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/libs/ ${IMGUI_DIR})
