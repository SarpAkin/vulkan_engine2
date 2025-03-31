#pragma once

#include <memory>

#include <vke/fwd.hpp>

#include "fwd.hpp"

#include "render/render_server.hpp"

namespace vke {

class GameEngine {
public:
    GameEngine(bool headless = false);
    ~GameEngine();

    void run();

    float get_delta_time() const { return m_delta_time; }
    Scene* get_scene() { return m_scene.get(); }
    RenderServer* get_render_server() { return m_render_server.get(); }

protected: // virtuals
    virtual void on_render(RenderServer::FrameArgs& args) { default_render(args); }
    virtual void on_update() {} // called before synchronizing with gpu.Good for physics

protected:
    void default_render(RenderServer::FrameArgs&);

private:

private:
    std::unique_ptr<vke::Scene> m_scene;
    //must be defined before the other render elements in order to be destroyed last
    std::unique_ptr<vke::RenderServer> m_render_server;
    std::unique_ptr<vke::DeferredRenderPipeline> m_render_pipeline;

    bool m_running     = true;
    float m_delta_time = 0.1f;
    float m_run_time   = 0.0f;
};

} // namespace vke