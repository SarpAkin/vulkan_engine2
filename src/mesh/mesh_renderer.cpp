#include "mesh_renderer.hpp"

#include "engine.hpp"

#include <vke/pipeline_loader.hpp>

#include <glm/glm.hpp>

namespace vke {

MeshRenderer::MeshRenderer(RenderServer* engine) : m_engine(engine) {
    auto pl = engine->get_pipeline_loader();

    m_default_pipeline = pl->load("vke::default");
}

struct DefaultShaderPC {
    glm::mat4 mvp;
};

void MeshRenderer::setup_for_render(vke::CommandBuffer& cmd) {
    cmd.bind_pipeline(m_default_pipeline.get());
}

void MeshRenderer::render_mesh(vke::CommandBuffer& cmd, Mesh* mesh, glm::mat4& transform) {
    cmd.bind_index_buffer(mesh->index_buffer.get(), mesh->index_type);
    cmd.bind_vertex_buffer({mesh->vertex_buffer.get()});

}
} // namespace vke