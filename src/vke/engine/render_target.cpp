#include "render_target.hpp"

#include "irender_system.hpp"

#include "../core/commandbuffer.hpp"

namespace vke {

struct RPRenderTargetFramelyData {
    std::unique_ptr<vke::CommandBuffer> subpass_cmd;
    std::unique_ptr<vke::CommandBuffer> compute_cmd;
};

RPRenderTarget::RPRenderTarget(RenderEngine* engine, Renderpass* rp, u32 subpass_index) : vke::System<struct RPRenderTargetFramelyData>(engine) {
    m_render_pass = rp;
    m_subpass_index = subpass_index;

    init_frame_datas([&](u32) {
        return RPRenderTargetFramelyData{
            .subpass_cmd = core()->create_cmd(false),
            .compute_cmd = core()->create_cmd(false),
        };
    });
}

RPRenderTarget::~RPRenderTarget() {
}

CommandBuffer* RPRenderTarget::get_draw_cmd() {
    return get_frame_data().subpass_cmd.get();
}
CommandBuffer* RPRenderTarget::get_compute_cmd() {
    return get_frame_data().compute_cmd.get();
}

void RPRenderTarget::subscribe(IRenderSystem* system) {
    system->on_subscribe(this);
    m_systems.push_back(system);
}

void RPRenderTarget::render_systems() {
    auto& fd = get_frame_data();
    fd.compute_cmd->reset();
    fd.subpass_cmd->reset();

    fd.compute_cmd->begin_secondry();
    fd.subpass_cmd->begin_secondry(m_render_pass, m_subpass_index);

    for(auto& system : m_systems){
        system->render(this);
    }

    fd.compute_cmd->end();
    fd.subpass_cmd->end();
}

} // namespace vke