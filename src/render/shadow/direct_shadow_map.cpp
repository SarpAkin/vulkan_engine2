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
    u32 depth     = builder.add_attachment(VK_FORMAT_D16_UNORM, VkClearValue{.depthStencil = {.depth = 1.0}});
    m_shadow_pass = builder.build(texture_size, texture_size);
    m_shadow_map  = m_shadow_pass->get_attachment_view(depth);

    m_render_server      = render_server;
    m_render_target_name = std::format("DirectShadowMapPass_{}", id_counter.fetch_add(1));

    auto& pgp_subpasses = m_render_server->get_pipeline_loader()->get_pipeline_globals_provider()->subpasses;
    if (!pgp_subpasses.contains(shadowD16)) {
        pgp_subpasses[shadowD16] = m_shadow_pass->get_subpass(0)->create_copy();

        auto* resource_manager = m_render_server->get_object_renderer()->get_resource_manager();
        resource_manager->add_pipeline2multi_pipeline(ObjectRenderer::pbr_pipeline_name, "vke::shadow::default");
    }

    m_render_server->get_object_renderer()->create_render_target(m_render_target_name, shadowD16);
}

DirectShadowMap::~DirectShadowMap() {
}

glm::mat4 DirectShadowMap::get_projection_view_matrix() { return m_camera->proj_view(); }
} // namespace vke