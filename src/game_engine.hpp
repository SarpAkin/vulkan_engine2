#pragma once

#include <memory>

#include <vke/fwd.hpp>

#include "fwd.hpp"

namespace vke {

class GameEngine {
public:
    GameEngine(bool headless = false);

    void run();

    float get_delta_time() const { return m_delta_time; }
    Scene* get_scene() { return m_scene.get(); }
    RenderServer* get_render_server() { return m_render_server.get(); }

protected: // virtuals
    virtual void on_render(vke::CommandBuffer& cmd) { default_render(cmd); }
    virtual void on_update() {} // called before synchronizing with gpu.Good for physics

protected:
    void default_render(vke::CommandBuffer& cmd);

private:

private:
    std::unique_ptr<vke::Scene> m_scene;
    std::unique_ptr<vke::RenderServer> m_render_server;

    bool m_running     = true;
    float m_delta_time = 0.1f;
    float m_run_time   = 0.0f;
};

} // namespace vke