#pragma once

#include "object_renderer.hpp"
#include "resource_manager.hpp"

namespace vke {

struct RenderState {
    vke::CommandBuffer& cmd;
    vke::CommandBuffer& compute_cmd;
    MaterialID bound_material_id                = 0;
    MeshID bound_mesh_id                        = 0;
    IPipeline* bound_pipeline                   = nullptr;
    const Mesh* mesh                            = nullptr;
    const ResourceManager::Material* material   = nullptr;
    ObjectRenderer::RenderTarget* render_target = nullptr;
};

} // namespace vke