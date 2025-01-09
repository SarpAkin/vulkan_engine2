#pragma once

#include <vke/fwd.hpp>

#include <glm/fwd.hpp>

#include "mesh.hpp"

namespace vke {

class Engine;

class MeshRenderer {
public:
    MeshRenderer(Engine* engine);

    void setup_for_render(vke::CommandBuffer& cmd);

    void render_mesh(vke::CommandBuffer& cmd,Mesh* mesh, glm::mat4& transform);

private:
    Engine* m_engine;
    std::unique_ptr<vke::IPipeline> m_default_pipeline;
};

} // namespace vke