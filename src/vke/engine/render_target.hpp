#pragma once

#include "irender_target.hpp"
#include "vk_system.hpp"

namespace vke {

class RPRenderTarget : public IRenderTarget, public vke::System<struct RPRenderTargetFramelyData> {
public:
    RPRenderTarget(RenderEngine* engine, Renderpass* rp, u32 subpass_index);
    ~RPRenderTarget();

    void set_camera(Camera* camera)override { m_camera = camera; }

    Renderpass* get_renderpass() override { return m_render_pass; }
    u32 get_subpass_index() override { return m_subpass_index; }
    Camera* get_camera() override { return m_camera; }

    CommandBuffer* get_draw_cmd() override;
    CommandBuffer* get_compute_cmd() override;

    void subscribe(IRenderSystem* system) override;
    void render_systems() override;

private:
    Renderpass* m_render_pass;
    u32 m_subpass_index;
    Camera* m_camera;

    std::vector<IRenderSystem*> m_systems;
};

} // namespace vke