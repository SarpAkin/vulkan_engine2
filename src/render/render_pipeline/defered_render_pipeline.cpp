#include "defered_render_pipeline.hpp"

#include <vke/vke_builders.hpp>

#include "../object_renderer/object_renderer.hpp"

namespace vke {

static DeferedRenderPipeline::DeferedRenderPass create_render_pass(Window* window) {
    auto builder = vke::RenderPassBuilder();
    auto albedo  = builder.add_attachment(VK_FORMAT_R8G8B8A8_SRGB, VkClearValue{.color = {}});
    auto normal  = builder.add_attachment(VK_FORMAT_A2R10G10B10_SNORM_PACK32, VkClearValue{.color = {}});
    auto depth   = builder.add_attachment(VK_FORMAT_D32_SFLOAT, VkClearValue{.depthStencil = {.depth = 1.0}});

    builder.add_subpass({albedo, normal}, depth, {});

    return DeferedRenderPipeline::DeferedRenderPass{
        .renderpass = builder.build(window->width(), window->height()),
        .albedo_id  = albedo,
        .normal_id  = normal,
        .depth_id   = depth,
    };
}

DeferedRenderPipeline::DeferedRenderPipeline(vke::RenderServer* render_server) {
    m_render_server = render_server;

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        m_framely.push_back(FramelyData{
            .compute_cmd = std::make_unique<CommandBuffer>(false),
            .gpass_cmd   = std::make_unique<CommandBuffer>(false),
        });
    }

    m_defered_render_pass = create_render_pass(render_server->get_window());
}

void DeferedRenderPipeline::render(RenderServer::FrameArgs& args) {
    auto& framely = get_framely();
    framely.compute_cmd->reset();
    framely.compute_cmd->begin_secondary();

    // framely.gpass_cmd->reset();
    // framely.gpass_cmd->begin_secondary(m_defered_render_pass->get_subpass(0));

    m_render_server->get_object_renderer()->render({
        .subpass_cmd        = args.main_pass_cmd,
        .compute_cmd        = framely.compute_cmd.get(),
        .render_target_name = "default",

    });

    framely.compute_cmd->end();
    // framely.gpass_cmd->end();

    auto& primary_cmd = *args.primary_cmd;

    primary_cmd.execute_secondaries(framely.compute_cmd.get());

    // m_defered_render_pass->set_external(true);
    // m_defered_render_pass->begin(primary_cmd);

    // m_defered_render_pass->set_external(false);
    // m_defered_render_pass->end(primary_cmd);
}

DeferedRenderPipeline::~DeferedRenderPipeline() {
}
} // namespace vke