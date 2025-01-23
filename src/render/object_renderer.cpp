#include "object_renderer.hpp"

#include "render_server.hpp"
#include "scene/camera.hpp"
#include "scene/components/transform.hpp"

#include <vke/pipeline_loader.hpp>

namespace vke {

void ObjectRenderer::render(vke::CommandBuffer& cmd) {
    auto view = m_registery->view<Renderable, Transform>();

    RenderState state = {.cmd = cmd};

    for (auto& e : view) {
        auto [r, t] = view.get(e);

        auto& model = m_render_models[r.model_id];

        struct Push {
            glm::mat4 mvp;
        };

        Push push{
            .mvp = m_camera->proj_view() * t.local_model_matrix(),
        };

        for (auto& part : model.parts) {
            if (!bind_material(state, part.material_id)) continue;
            if (!bind_mesh(state, part.mesh_id)) continue;

            auto* mesh = state.mesh;

            cmd.push_constant(&push);
            cmd.draw_indexed(mesh->index_count, 1, 0, 0, 0);
        }
    }
}

bool ObjectRenderer::bind_mesh(RenderState& state, MeshID id) {
    if (state.bound_mesh_id == id) return true;

    if (auto it = m_meshes.find(id); it != m_meshes.end()) {
        state.mesh = &it->second;
    } else {
        LOG_ERROR("failed to bind mesh with id %d", id.id);
        return false;
    }

    state.cmd.bind_index_buffer(state.mesh->index_buffer.get(), state.mesh->index_type);
    state.cmd.bind_vertex_buffer({state.mesh->vertex_buffer.get()});

    return true;
}

bool ObjectRenderer::bind_material(RenderState& state, MaterialID id) {
    if (state.bound_material_id == id) return true;

    if (auto it = m_materials.find(id); it != m_materials.end()) {
        state.material = &it->second;
    } else {
        LOG_ERROR("failed to bind material with id %d", id.id);
        return false;
    }

    if (state.bound_pipeline != state.material->pipeline.get()) {
        state.bound_pipeline = state.material->pipeline.get();
        state.cmd.bind_pipeline(state.bound_pipeline);
    }
    // state.cmd.bind_descriptor_set(MATERIAL_SET, state.material->material_set);

    return true;
}

MaterialID ObjectRenderer::create_material(const std::string& pipeline_name, const std::vector<ImageID>& images, const std::string& material_name) {
    Material m{
        .pipeline     = m_render_server->get_pipeline_loader()->load(pipeline_name.c_str()),
        .material_set = VK_NULL_HANDLE,
        .images       = images,
        .name         = material_name,
    };

    auto id = MaterialID(new_raw_id());

    if (!material_name.empty()) {
        assert(!m_material_names2material_ids.contains(material_name) && "material name is already present");
        m_material_names2material_ids[material_name] = id;
    }

    m_materials[id] = std::move(m);
    return id;
}

MeshID ObjectRenderer::create_mesh(Mesh mesh, const std::string& name) {
    auto id = MeshID(new_raw_id());

    m_meshes[id] = std::move(mesh);

    if (!name.empty()) {
        assert(!m_mesh_names2mesh_ids.contains(name) && "mesh name is already present");
        m_mesh_names2mesh_ids[name] = id;
    }

    return id;
}

RenderModelID ObjectRenderer::create_model(MeshID mesh, MaterialID material, const std::string& name) {
    auto id = RenderModelID(new_raw_id());

    m_render_models[id] = RenderModel{
        .parts = {
            {mesh, material},
        },
    };

    if (!name.empty()) {
        bind_name2model(id, name);
    }

    return id;
}

RenderModelID ObjectRenderer::create_model(const std::vector<std::pair<MeshID, MaterialID>>& parts, const std::string& name) {
    auto id = RenderModelID(new_raw_id());

    m_render_models[id] = RenderModel{
        .parts = map_vec(parts, [](auto& part) {
        auto& [mesh, mat] = part;
        return RenderModel::Part{mesh, mat};
    }),
    };

    if (!name.empty()) {
        bind_name2model(id, name);
    }

    return id;
}

void ObjectRenderer::bind_name2model(RenderModelID id, const std::string& name) {
    assert(!m_render_model_names2model_ids.contains(name) && "model name is already present");

    m_render_model_names2model_ids[name] = id;
    m_render_models[id].name             = name;
}

} // namespace vke
