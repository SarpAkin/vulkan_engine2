#include "defered_render_pipeline.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/vke_builders.hpp>

#include "../object_renderer/light_buffers_manager.hpp"
#include "../object_renderer/object_renderer.hpp"
#include "../object_renderer/resource_manager.hpp"

#include "../object_renderer/hierarchical_z_buffers.hpp"

#include "render/debug/line_drawer.hpp"
#include "render/debug/gpu_timing_system.hpp"

#include "../shadow/shadow_manager.hpp"

namespace vke {

static DeferredRenderPipeline::DeferredRenderPass create_render_pass(Window* window) {
    auto builder = vke::RenderPassBuilder();
    auto albedo  = builder.add_attachment(VK_FORMAT_R8G8B8A8_SRGB, VkClearValue{.color = {}}, true);
    auto normal  = builder.add_attachment(VK_FORMAT_R8G8B8A8_SNORM, VkClearValue{.color = {}}, true);
    auto depth   = builder.add_attachment(VK_FORMAT_D32_SFLOAT, VkClearValue{.depthStencil = {.depth = 0.0}}, true);

    builder.add_subpass({albedo, normal}, depth, {});

    auto render_pass = builder.build(window->width(), window->height());

    auto& pipeline_render_target_description = render_pass->get_render_target_description(0);

    pipeline_render_target_description.depth_compare_op = VK_COMPARE_OP_GREATER_OR_EQUAL;

    return DeferredRenderPipeline::DeferredRenderPass{
        .renderpass         = std::move(render_pass),
        .albedo_id          = albedo,
        .normal_id          = normal,
        .depth_id           = depth,
        .subpass_name       = "vke::gpass",
        .render_target_name = "vke::gpass",
    };
}

DeferredRenderPipeline::DeferredRenderPipeline(vke::RenderServer* render_server) {
    m_render_server = render_server;

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        m_framely.push_back(FramelyData{
            .compute_cmd = std::make_unique<CommandBuffer>(false),
            .gpass_cmd   = std::make_unique<CommandBuffer>(false),
        });
    }

    m_deferred_render_pass = create_render_pass(render_server->get_window());
    auto pg_provider       = m_render_server->get_pipeline_loader()->get_pipeline_globals_provider();
    pg_provider->subpasses.emplace(
        m_deferred_render_pass.subpass_name,
        m_deferred_render_pass.renderpass->get_subpass(0)->create_copy());

    vke::DescriptorSetLayoutBuilder builder;
    builder.add_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
    builder.add_ubo(VK_SHADER_STAGE_FRAGMENT_BIT);
    builder.add_ssbo(VK_SHADER_STAGE_FRAGMENT_BIT);
    builder.add_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1); // shadows
    m_deferred_set_layout = builder.build();

    pg_provider->set_layouts.emplace("vke::deferred_render_set", m_deferred_set_layout);

    auto object_renderer  = m_render_server->get_object_renderer();
    auto resource_manager = object_renderer->get_resource_manager();
    resource_manager->add_pipeline2multi_pipeline("vke::object_renderer::pbr_pipeline", "vke::gpass::default");

    object_renderer->create_render_target(m_deferred_render_pass.render_target_name, m_deferred_render_pass.subpass_name,
        {
            .allow_indirect_render = true,
            .allow_hzb_culling     = true,
        });

    m_deferred_pipeline = m_render_server->get_pipeline_loader()->load("vke::post_deferred");

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        create_set(i, false);
    }

    create_hzb();
}

void DeferredRenderPipeline::render(RenderServer::FrameArgs& args) {
    check_for_resize(*args.primary_cmd);

    auto& framely = get_framely();
    framely.compute_cmd->reset();
    framely.compute_cmd->begin_secondary();

    framely.gpass_cmd->reset();

    auto* deferred_pass = m_deferred_render_pass.renderpass.get();

    framely.gpass_cmd->begin_secondary(deferred_pass->get_subpass(0));

    m_render_server->get_object_renderer()->render({
        .subpass_cmd        = framely.gpass_cmd.get(),
        .compute_cmd        = framely.compute_cmd.get(),
        .render_target_name = m_deferred_render_pass.render_target_name,
    });

    m_render_server->get_line_drawer()->flush(*framely.gpass_cmd, m_camera, m_deferred_render_pass.subpass_name);

    framely.compute_cmd->end();
    framely.gpass_cmd->end();

    auto& primary_cmd = *args.primary_cmd;

    primary_cmd.execute_secondaries(framely.compute_cmd.get());

    deferred_pass->set_external(true);
    deferred_pass->begin(primary_cmd);

    primary_cmd.execute_secondaries(framely.gpass_cmd.get());

    deferred_pass->set_external(false);
    deferred_pass->end(primary_cmd);

    auto* timer =  m_render_server->get_gpu_timing_system();
    timer->timestamp(*args.main_pass_cmd, "pre deferred", VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    args.main_pass_cmd->bind_pipeline(m_deferred_pipeline.get());
    args.main_pass_cmd->bind_descriptor_set(0, m_deferred_set[m_render_server->get_frame_index()]);

    args.main_pass_cmd->draw(3, 1, 0, 0);
}

DeferredRenderPipeline::~DeferredRenderPipeline() {
}

void DeferredRenderPipeline::create_set(int index, bool update) {
    IImageView* images[] = {
        m_deferred_render_pass.renderpass->get_attachment_view(m_deferred_render_pass.albedo_id),
        m_deferred_render_pass.renderpass->get_attachment_view(m_deferred_render_pass.normal_id),
        m_deferred_render_pass.renderpass->get_attachment_view(m_deferred_render_pass.depth_id),
    };

    auto* object_renderer = m_render_server->get_object_renderer();
    auto* light_manager   = object_renderer->get_light_manager();
    auto* shadow_manager  = light_manager->get_shadow_manager();

    VkSampler sampler = object_renderer->get_resource_manager()->get_nearest_sampler();

    IImageView* shadow_maps[] = {
        shadow_manager->get_direct_shadow_map_texture(0).get(),
    };

    vke::DescriptorSetBuilder builder;
    builder.add_image_samplers(images, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler, VK_SHADER_STAGE_FRAGMENT_BIT);
    builder.add_ubo(object_renderer->get_view_buffer(m_deferred_render_pass.render_target_name, index), VK_SHADER_STAGE_FRAGMENT_BIT);
    builder.add_ssbo(light_manager->get_get_lights_buffer(), VK_SHADER_STAGE_FRAGMENT_BIT);
    builder.add_image_samplers(shadow_maps, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shadow_manager->get_shadow_sampler(), VK_SHADER_STAGE_FRAGMENT_BIT);

    if (!update) {
        m_deferred_set[index] = builder.build(m_render_server->get_descriptor_pool(), m_deferred_set_layout);
    } else {
        builder.update_set(m_deferred_set[index], m_deferred_set_layout);
    }
}

void DeferredRenderPipeline::set_camera(Camera* camera) {
    m_camera = camera;
    m_render_server->get_object_renderer()->set_camera(m_deferred_render_pass.render_target_name, camera);
}

void DeferredRenderPipeline::check_for_resize(vke::CommandBuffer& cmd) {
    // return if the extends of renderpass and the window are same
    auto w_extends = m_render_server->get_window()->extend();

    if (!is_equal(m_deferred_render_pass.renderpass->extend(), w_extends)) {
        m_deferred_render_pass.renderpass->resize(cmd, w_extends.width, w_extends.height);
        
        create_hzb();
        m_sets_needing_update.set();
    }

    int frame_index = m_render_server->get_frame_index();

    if (m_sets_needing_update[frame_index]) {
        create_set(frame_index, true);
        m_sets_needing_update[frame_index] = false;
    }
}

void DeferredRenderPipeline::create_hzb() {
    m_hzb = std::make_unique<HierarchicalZBuffers>(m_render_server,m_deferred_render_pass.renderpass->get_attachment_view(m_deferred_render_pass.depth_id));
    m_render_server->get_object_renderer()->set_hzb(m_deferred_render_pass.render_target_name, m_hzb.get());

}
} // namespace vke