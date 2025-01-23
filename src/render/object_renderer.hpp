#pragma once

#include <unordered_map>
#include <vector>

#include <vke/util.hpp>
#include <vke/vke.hpp>

#include "iobject_renderer.hpp"
#include "mesh/mesh.hpp"

#include "fwd.hpp"

namespace vke {

class ObjectRenderer final : public IObjectRenderer {
public:
    constexpr static int MATERIAL_SET = 0;

public:
    ObjectRenderer() {}

    void set_render_server(RenderServer* render_server) override { m_render_server = render_server; };
    void set_entt_registery(entt::registry* registery) override { m_registery = registery; }
    void render(vke::CommandBuffer& cmd) override;

    void set_camera(Camera* camera) { m_camera = camera; }

    MaterialID create_material(const std::string& pipeline_name, const std::vector<ImageID>& images = {}, const std::string& material_name = "");
    MeshID create_mesh(Mesh mesh, const std::string& name = "");
    RenderModelID create_model(MeshID mesh, MaterialID material, const std::string& name = "");
    RenderModelID create_model(const std::vector<std::pair<MeshID, MaterialID>>& parts, const std::string& name = "");

    void bind_name2model(RenderModelID id, const std::string& name);

    RenderModelID get_model_id(const std::string& name) const { return m_render_model_names2model_ids.at(name); }


private:
    struct RenderModel {
        struct Part {
            MeshID mesh_id;
            MaterialID material_id;
        };

        vke::SmallVec<Part> parts;
        std::string name;
    };

    struct Material {
        vke::RCResource<vke::IPipeline> pipeline;
        VkDescriptorSet material_set;
        vke::SmallVec<ImageID> images;
        std::string name;
    };

    struct RenderState {
        vke::CommandBuffer& cmd;
        MaterialID bound_material_id = 0;
        MeshID bound_mesh_id         = 0;
        IPipeline* bound_pipeline    = nullptr;
        Mesh* mesh                   = nullptr;
        Material* material           = nullptr;
    };

private:
    uint32_t new_raw_id() { return m_id_counter++; }

    bool bind_mesh(RenderState& state, MeshID id);
    bool bind_material(RenderState& state, MaterialID id);

private:
    std::unordered_map<ImageID, std::unique_ptr<IImageView>> m_images;
    std::unordered_map<MaterialID, Material> m_materials;
    std::unordered_map<RenderModelID, RenderModel> m_render_models;
    std::unordered_map<MeshID, Mesh> m_meshes;

    std::unordered_map<std::string, MaterialID> m_material_names2material_ids;
    std::unordered_map<std::string, MeshID> m_mesh_names2mesh_ids;
    std::unordered_map<std::string, RenderModelID> m_render_model_names2model_ids;

    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;
    VkDescriptorSetLayout m_material_set_layout;

    vke::RenderServer* m_render_server;
    entt::registry* m_registery;
    Camera* m_camera;

    uint32_t m_id_counter = 1; // 0 is null

    RenderModelID m_cube_model = 0;
};

} // namespace vke