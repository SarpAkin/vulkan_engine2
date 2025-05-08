#include "direct_shadow_map.hpp"

#include <format>
#include <vke/pipeline_loader.hpp>

#include "render/object_renderer/object_renderer.hpp"
#include "render/object_renderer/resource_manager.hpp"
#include "render/render_server.hpp"

#include "scene/camera.hpp"

namespace vke {

std::atomic<uint32_t> id_counter;

DirectShadowMap::DirectShadowMap(RenderServer* render_server, u32 texture_size) {
    vke::RenderPassBuilder builder;
    u32 depth = builder.add_attachment(VK_FORMAT_D16_UNORM, VkClearValue{.depthStencil = {.depth = 0.0}}, true);
    builder.add_subpass({}, depth, {});
    m_shadow_pass = builder.build(texture_size, texture_size);
    m_shadow_map  = m_shadow_pass->get_attachment_view(depth);

    m_render_server      = render_server;
    m_object_renderer    = m_render_server->get_object_renderer();
    m_render_target_name = std::format("DirectShadowMapPass_{}", id_counter.fetch_add(1));

    auto& pgp_subpasses = m_render_server->get_pipeline_loader()->get_pipeline_globals_provider()->subpasses;
    if (!pgp_subpasses.contains(shadowD16)) {
        m_shadow_pass->get_render_target_description(0).depth_compare_op = VK_COMPARE_OP_GREATER_OR_EQUAL;

        pgp_subpasses[shadowD16] = m_shadow_pass->get_subpass(0)->create_copy();

        auto* resource_manager = m_render_server->get_object_renderer()->get_resource_manager();
        resource_manager->add_pipeline2multi_pipeline(ObjectRenderer::pbr_pipeline_name, "vke::shadowD16::default");
    }

    m_render_server->get_object_renderer()->create_render_target(m_render_target_name, shadowD16, true);

    m_camera = std::make_unique<vke::OrthographicCamera>();
    m_object_renderer->set_camera(m_render_target_name, m_camera.get());
}

DirectShadowMap::~DirectShadowMap() {
}

glm::mat4 DirectShadowMap::get_projection_view_matrix(u32, u32) { return m_camera->proj_view(); }

void DirectShadowMap::render(vke::CommandBuffer& primary_buffer, u32) {
    RCResource<vke::CommandBuffer> shadow_pass_cmd = m_render_server->get_framely_command_pool()->allocate(false);

    shadow_pass_cmd->begin_secondary(m_shadow_pass->get_subpass(0));

    m_object_renderer->render(RenderArguments{
        .subpass_cmd        = shadow_pass_cmd.get(),
        .compute_cmd        = &primary_buffer,
        .render_target_name = m_render_target_name,
    });

    shadow_pass_cmd->end();

    m_shadow_pass->set_external(true);
    m_shadow_pass->begin(primary_buffer);

    primary_buffer.execute_secondaries(shadow_pass_cmd.get());
    m_shadow_pass->end(primary_buffer);

    primary_buffer.add_execution_dependency(shadow_pass_cmd->get_reference());
}

void DirectShadowMap::set_camera_data(const ShadowMapCameraData& camera_data, u32) {
    m_camera->set_world_pos(camera_data.position);
    m_camera->set_rotation(camera_data.direction, camera_data.up);
    m_camera->z_far       = camera_data.far;
    m_camera->z_near      = 0.1f;
    m_camera->half_height = camera_data.height / 2.0f;
    m_camera->half_width  = camera_data.width / 2.0f;
    m_camera->update();
}

glm::dvec3 DirectShadowMap::get_camera_position(u32 index) {
    return m_camera->get_world_pos();
}

glm::vec3 DirectShadowMap::get_camera_direction(u32 index, u32 view_index) {
    return m_camera->forward();
}
} // namespace vke