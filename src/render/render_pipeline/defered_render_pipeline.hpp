#pragma once

#include <bitset>
#include <vke/vke.hpp>

#include "fwd.hpp"
#include "render/render_server.hpp"

namespace vke {

class DeferredRenderPipeline {
public:
    struct DeferredRenderPass {
        std::unique_ptr<vke::Renderpass> renderpass;
        u32 albedo_id, normal_id, depth_id;
        std::string subpass_name, render_target_name;
    };

    DeferredRenderPipeline(vke::RenderServer* render_server);
    ~DeferredRenderPipeline();

public:
    void render(RenderServer::FrameArgs& frame_args);
    void set_camera(Camera* camera);

private:
    struct FramelyData {
        std::unique_ptr<vke::CommandBuffer> compute_cmd, gpass_cmd;
    };

private:
    FramelyData& get_framely() { return m_framely[m_render_server->get_frame_index()]; }

    void create_set(int index, bool update = false);
    void check_for_resize(vke::CommandBuffer& cmd);

    void create_hzb();

private:
    std::vector<FramelyData> m_framely;
    DeferredRenderPass m_deferred_render_pass;

    vke::RenderServer* m_render_server;
    vke::Camera* m_camera = nullptr;

    VkDescriptorSetLayout m_deferred_set_layout;
    VkDescriptorSet m_deferred_set[FRAME_OVERLAP];
    std::bitset<FRAME_OVERLAP> m_sets_needing_update;

    vke::RCResource<vke::IPipeline> m_deferred_pipeline;
    vke::RCResource<vke::HierarchicalZBuffers> m_hzb;
};

} // namespace vke