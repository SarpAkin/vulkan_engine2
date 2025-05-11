#include "shadow_manager.hpp"

#include <vke/vke.hpp>

#include "direct_shadow_map.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "imgui.h"
#include "ishadow_map.hpp"
#include "shadow_utils.hpp"

#include "render/debug/line_drawer.hpp"
#include "render/render_server.hpp"

#include "game_engine.hpp"
#include "scene/camera.hpp"
#include "scene/scene.hpp"

namespace vke {

u32 shadow_map_count         = 4;
static bool update_proj_view = true;

void ShadowManager::render_shadows(vke::CommandBuffer& cmd) {
    if (m_debug_menu_enabled) debug_menu();
    if (m_debug_draw_frustums) debug_draw_frustums();

    if (update_proj_view) {
        
        std::vector<IShadowMap::LateRasterData> raster_buffers;
        
        for (int i = 0; i < shadow_map_count; i++) {
            if (m_shadow_map->requires_rerender(i)) {
                m_shadow_map->render(cmd, i,&raster_buffers);
            }
        }

        IShadowMap::execute_late_rasters(cmd, raster_buffers);
    }
}

ShadowManager::ShadowManager(RenderServer* render_server) {
    m_render_server = render_server;

    m_shadow_map = std::make_unique<vke::DirectShadowMap>(m_render_server, 4096, shadow_map_count);
    m_shadow_map->set_camera_data({
        .position  = {-100, 100, 100},
        .direction = glm::normalize(glm::vec3(1, -1, 1)),
        .far       = 400,
        .width     = 100,
        .height    = 100,
    });

    VkFilter filter = VK_FILTER_LINEAR;
    VkSamplerCreateInfo sampler_info{
        .sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter     = filter,
        .minFilter     = filter,
        .addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
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
        ImGui::Checkbox("update proj view", &update_proj_view);

        ImGui::EndMenu();
    };
}

float worldDepthToNormalized(float depth, float near, float far) {
    // Calculate the normalized depth (1.0 at near, 0.0 at far)
    return (far - depth) / (far - near);
}

void ShadowManager::debug_draw_frustums() {
    auto* line_drawer = m_render_server->get_line_drawer();

    u32 color = 0xFF'FF'FF'FF;
    for (int i = 0; i < m_shadow_map->get_shadow_map_array_size(); i++) {
        glm::mat4 proj_view = m_shadow_map->get_projection_view_matrix(i);
        line_drawer->draw_camera_frustum(glm::inverse(proj_view), color);
        glm::vec3 camera_pos = m_shadow_map->get_camera_position(i);
        glm::vec3 camera_dir = m_shadow_map->get_camera_direction(i);
        line_drawer->draw_line(camera_pos, camera_pos + camera_dir * 10.f, 0x00'FF'FF'00);
    }
}

void ShadowManager::update_direct_cascaded_shadows(u32 index, glm::vec3 direction) {
    if (!update_proj_view) return;

    auto* player_camera = dynamic_cast<PerspectiveCamera*>(GameEngine::get_instance()->get_scene()->get_camera());
    u64 frame_index     = GameEngine::get_instance()->get_frame_counter();

    float base_z_distance = 500.f / (std::pow(2.f, shadow_map_count) - 1.f);
    float prev_z          = 1.0f;
    float prev_world_far  = 0.f;

    auto calculate_clip_z = [&](float world_far) {
        glm::vec4 vpos = player_camera->projection() * glm::vec4(0, 0, world_far, 1.0);
        return vpos.z / vpos.w;
    };

    float shadow_far = 1000.f;

    bool updated_cascades[4]                = {true, false, false, false};
    updated_cascades[(frame_index % 3) + 1] = true;

    m_min_z_for_csm.resize(shadow_map_count);

    for (int i = 0; i < shadow_map_count; i++) {
        float world_far = std::pow(2.f, i) * base_z_distance + prev_world_far;
        auto z          = calculate_clip_z(-world_far);

        m_min_z_for_csm[i] = z;

        if (updated_cascades[i]) {
            auto shadow_data = calculate_optimal_direct_shadow_map_frustum(
                glm::inverse(player_camera->proj_view()),
                z, prev_z,
                glm::normalize(glm::vec3(1, -1, 1)), //,
                shadow_far                           //
            );

            m_shadow_map->set_camera_data(shadow_data, i);
        }

        prev_z         = z;
        prev_world_far = world_far;
    }

    // float excess_z = shadow_far - shadow_data.far;
    // shadow_data.position += shadow_data.direction * -excess_z;
    // shadow_data.far = 1000.f;
}

} // namespace vke