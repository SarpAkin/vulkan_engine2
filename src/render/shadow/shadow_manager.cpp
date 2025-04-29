#include "shadow_manager.hpp"

#include <vke/vke.hpp>

#include "direct_shadow_map.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "imgui.h"
#include "ishadow_map.hpp"

#include "render/debug/line_drawer.hpp"
#include "render/render_server.hpp"

namespace vke {

void ShadowManager::render_shadows(vke::CommandBuffer& cmd) {
    if (m_debug_menu_enabled) debug_menu();
    if (m_debug_draw_frustums) debug_draw_frustums();

    m_shadow_map->render(cmd);
}

ShadowManager::ShadowManager(RenderServer* render_server) {
    m_render_server = render_server;

    m_shadow_map = std::make_unique<vke::DirectShadowMap>(m_render_server, 8192);
    m_shadow_map->set_camera_data({
        .position  = {-100, 100, 100},
        .direction = glm::normalize(glm::vec3(1, -1, 1)),
        .far       = 400,
        .width     = 100,
        .height    = 100,
    });

    VkSamplerCreateInfo sampler_info{
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_LINEAR,
        .minFilter    = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .compareEnable = true,
        // reverse z is enabled for all
        .compareOp   = VK_COMPARE_OP_GREATER_OR_EQUAL,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
    };

    vkCreateSampler(vke::VulkanContext::get_context()->get_device(), &sampler_info, nullptr, &m_shadow_sampler);
}

ShadowManager::~ShadowManager() {
    vkDestroySampler(vke::VulkanContext::get_context()->get_device(), m_shadow_sampler, nullptr);
}
vke::RCResource<IImageView> ShadowManager::get_direct_shadow_map_texture(u32 index) { return m_shadow_map->get_image_view(); }

void ShadowManager::debug_menu() {
    if (ImGui::BeginMenu("Shadow Manager", m_debug_menu_enabled)) {
        ImGui::Checkbox("draw shadow frustums", &m_debug_draw_frustums);

        ImGui::EndMenu();
    };
}

void ShadowManager::debug_draw_frustums() {
    auto* line_drawer = m_render_server->get_line_drawer();

    u32 color           = 0xFF'FF'FF'FF;
    glm::mat4 proj_view = m_shadow_map->get_projection_view_matrix();
    line_drawer->draw_camera_frustum(glm::inverse(proj_view), color);
    glm::vec3 camera_pos = m_shadow_map->get_camera_position();
    glm::vec3 camera_dir = m_shadow_map->get_camera_direction();
    line_drawer->draw_line(camera_pos, camera_pos + camera_dir * 10.f, 0x00'FF'FF'00);
}

} // namespace vke