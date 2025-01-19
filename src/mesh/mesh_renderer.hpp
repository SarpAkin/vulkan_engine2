#pragma once

#include <vke/fwd.hpp>

#include <glm/fwd.hpp>

#include "mesh.hpp"

namespace vke {

class RenderServer;

class  MeshRenderer {
public:
    MeshRenderer(RenderServer* engine);

    void setup_for_render(vke::CommandBuffer& cmd);

    void render_mesh(vke::CommandBuffer& cmd,Mesh* mesh, glm::mat4& transform);

    std::unique_ptr<vke::IPipeline> m_default_pipeline;
private:
    RenderServer* m_engine;
};

} // namespace vke