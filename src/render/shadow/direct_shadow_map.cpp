#include "direct_shadow_map.hpp"

#include <format>
#include <vke/pipeline_loader.hpp>

#include "render/object_renderer/hierarchical_z_buffers.hpp"
#include "render/object_renderer/object_renderer.hpp"
#include "render/object_renderer/resource_manager.hpp"
#include "render/render_server.hpp"

#include "scene/camera.hpp"

namespace vke {

std::atomic<uint32_t> id_counter;

DirectShadowMap::DirectShadowMap(RenderServer* render_server, u32 texture_size, u32 layers) {
    vke::RenderPassBuilder builder;
    builder.set_layer_count(layers);
    m_layer_count = layers;
    m_shadow_maps_waiting_for_rerender.resize(layers, true);
    u32 depth = builder.add_attachment(VK_FORMAT_D16_UNORM, VkClearValue{.depthStencil = {.depth = 0.0}}, true);
    builder.add_subpass({}, depth, {});
    m_shadow_pass = builder.build(texture_size, texture_size);
    m_shadow_map  = m_shadow_pass->get_attachment_view(depth);

    m_render_server   = render_server;
    m_object_renderer = m_render_server->get_object_renderer();

    auto& pgp_subpasses = m_render_server->get_pipeline_loader()->get_pipeline_globals_provider()->subpasses;
    if (!pgp_subpasses.contains(shadowD16)) {
        m_shadow_pass->get_render_target_description(0).depth_compare_op = VK_COMPARE_OP_GREATER_OR_EQUAL;

        pgp_subpasses[shadowD16] = m_shadow_pass->get_subpass(0)->create_copy();

        auto* resource_manager = m_render_server->get_object_renderer()->get_resource_manager();
        resource_manager->add_pipeline2multi_pipeline(ObjectRenderer::pbr_pipeline_name, "vke::shadowD16::default");
    }

    u32 base_shadow_map_index = id_counter.fetch_add(1);
    for (int i = 0; i < layers; i++) {

        auto render_target_name = std::format("DirectShadowMapPass_{}:{}", base_shadow_map_index, i);

        m_render_server->get_object_renderer()->create_render_target(render_target_name, shadowD16, {true});

        auto camera = std::make_unique<vke::OrthographicCamera>();
        m_object_renderer->set_camera(render_target_name, camera.get());

        m_cameras.push_back(std::move(camera));

        if(false){
            auto sub_view = dynamic_cast<vke::Image*>(m_shadow_map.get())->create_subview(SubViewArgs{
                .base_layer  = static_cast<u32>(i),
                .layer_count = 1,
                .view_type   = VK_IMAGE_VIEW_TYPE_2D,
            });
    
            m_hz_buffers.push_back(std::make_unique<HierarchicalZBuffers>(m_render_server, sub_view.get()));
            m_sub_views.push_back(std::move(sub_view));
            m_object_renderer->set_hzb(render_target_name, m_hz_buffers.back().get());
        }

        m_render_target_names.push_back(std::move(render_target_name));
    }
}

DirectShadowMap::~DirectShadowMap() {
}

glm::mat4 DirectShadowMap::get_projection_view_matrix(u32 i, u32) { return m_cameras[i]->proj_view(); }

void DirectShadowMap::render(vke::CommandBuffer& primary_buffer, u32 layer_index, std::vector<LateRasterData>* raster_buffers) {
    assert(layer_index < m_layer_count);

    RCResource<vke::CommandBuffer> shadow_pass_cmd = m_render_server->get_framely_command_pool()->allocate(false);

    m_shadow_pass->set_active_frame_buffer_instance(layer_index);
    shadow_pass_cmd->begin_secondary(m_shadow_pass->get_subpass(0));

    m_object_renderer->render(RenderArguments{
        .subpass_cmd        = shadow_pass_cmd.get(),
        .compute_cmd        = &primary_buffer,
        .render_target_name = m_render_target_names[layer_index],
    });

    shadow_pass_cmd->end();

    if (raster_buffers) {
        raster_buffers->push_back(LateRasterData{
            .render_buffer     = std::move(shadow_pass_cmd),
            .shadow_renderpass = m_shadow_pass.get(),
            .layer_index       = layer_index,
        });
    } else {
        m_shadow_pass->set_external(true);
        m_shadow_pass->begin(primary_buffer);

        primary_buffer.execute_secondaries(shadow_pass_cmd.get());
        m_shadow_pass->end(primary_buffer);

        primary_buffer.add_execution_dependency(shadow_pass_cmd->get_reference());
    }

    m_shadow_maps_waiting_for_rerender[layer_index] = false;
}

void DirectShadowMap::set_camera_data(const ShadowMapCameraData& camera_data, u32 layer_index) {
    auto* cam = m_cameras[layer_index].get();

    cam->set_world_pos(camera_data.position);
    cam->set_rotation(camera_data.direction, camera_data.up);
    cam->z_far       = camera_data.far;
    cam->z_near      = 0.1f;
    cam->half_height = camera_data.height / 2.0f;
    cam->half_width  = camera_data.width / 2.0f;
    cam->update();

    m_shadow_maps_waiting_for_rerender[layer_index] = true;
}

glm::dvec3 DirectShadowMap::get_camera_position(u32 index) {
    return m_cameras[index]->get_world_pos();
}

glm::vec3 DirectShadowMap::get_camera_direction(u32 index, u32 view_index) {
    return m_cameras[index]->forward();
}
} // namespace vke