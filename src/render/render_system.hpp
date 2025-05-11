#pragma once

#include <memory>

#include "fwd.hpp"
#include "render/render_server.hpp"

namespace vke {

class RenderSystem {
public:
    RenderSystem(GameEngine* game_engine);
    ~RenderSystem();

public:
    RenderServer* get_render_server() { return m_render_server.get(); }

    void render(RenderServer::FrameArgs& frame_args);

private:
    vke::GameEngine* m_game_engine;
    vke::Scene* m_scene;
    //must be defined before the other render elements in order to be destroyed last
    std::unique_ptr<vke::RenderServer> m_render_server;
    std::unique_ptr<vke::DeferredRenderPipeline> m_render_pipeline;
};

}; // namespace vke