#pragma once

#include "irender_target.hpp"
#include "vk_system.hpp"

namespace vke {

class RPRenderTarget : public IRenderTarget, public vke::System<struct RPRenderTargetFramelyData> {
public:
    RPRenderTarget(RenderEngine* engine, Renderpass* rp, u32 subpass_index, RenderTargetType type = RenderTargetType::UNDEFINED);
    ~RPRenderTarget();

    void set_camera(Camera* camera) override { m_camera = camera; }

    RenderTargetType get_render_target_type() const override { return m_type; }
    Renderpass* get_renderpass() override { return m_render_pass; }
    u32 get_subpass_index() override { return m_subpass_index; }
    Camera* get_camera() override { return m_camera; }

    CommandBuffer* get_draw_cmd() override;
    CommandBuffer* get_compute_cmd() override;

    void subscribe(IRenderSystem* system) final override;
    void render_systems() final override;


protected:
    Renderpass* m_render_pass;
    u32 m_subpass_index;
    RenderTargetType m_type;
    Camera* m_camera;

private:
    std::vector<IRenderSystem*> m_systems;
};

} // namespace vke