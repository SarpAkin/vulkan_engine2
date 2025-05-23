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
    RenderServer* get_render_server();
    RenderSystem* get_renderer() { return m_renderer.get(); }

    double get_runtime() const { return m_run_time; }
    u64 get_frame_counter() const {return m_frame_counter;}

    static GameEngine* get_instance();

protected: // virtuals
    virtual void on_render(RenderServer::FrameArgs& args) { default_render(args); }
    virtual void on_update() {} // called before synchronizing with gpu.Good for physics

protected:
    void default_render(RenderServer::FrameArgs&);

private:
private:
    std::unique_ptr<vke::Scene> m_scene;
    std::unique_ptr<vke::RenderSystem> m_renderer;

    bool m_running     = true;
    float m_delta_time = 0.1f;
    double m_run_time  = 0.0f;
    u64 m_frame_counter = 0;
};

} // namespace vke