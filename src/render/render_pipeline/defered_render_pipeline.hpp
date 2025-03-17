#pragma once

#include <vke/vke.hpp>

#include "fwd.hpp"
#include "render/render_server.hpp"

namespace vke {

class DeferedRenderPipeline {
public:
    struct DeferedRenderPass {
        std::unique_ptr<vke::Renderpass> renderpass;
        u32 albedo_id, normal_id, depth_id;
    };

    DeferedRenderPipeline(vke::RenderServer* render_server);
    ~DeferedRenderPipeline();

public:
    void render(RenderServer::FrameArgs& frame_args);

private:
    struct FramelyData {
        std::unique_ptr<vke::CommandBuffer> compute_cmd, gpass_cmd;
    };

private:
    FramelyData& get_framely() { return m_framely[m_render_server->get_frame_index()]; }

private:
    std::vector<FramelyData> m_framely;
    DeferedRenderPass m_defered_render_pass;

    vke::RenderServer* m_render_server;
};

} // namespace vke