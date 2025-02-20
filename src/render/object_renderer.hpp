#pragma once

#include <unordered_map>
#include <vector>

#include <vke/util.hpp>
#include <vke/vke.hpp>

#include "common.hpp"
#include "iobject_renderer.hpp"
#include "mesh/mesh.hpp"

#include "fwd.hpp"

namespace vke {

class ObjectRenderer final : public IObjectRenderer, DeviceGetter {
public:
    constexpr static int SCENE_SET    = 0;
    constexpr static int MATERIAL_SET = 1;
    constexpr static int LIGHT_SET    = 2;

    constexpr static int MATERIAL_SET_IMAGE_COUNT = 4;

public:
    ObjectRenderer(RenderServer* render_server);
    ~ObjectRenderer();

    void set_entt_registery(entt::registry* registery) override { m_registry = registery; }
    void set_scene(Scene* scene);
    void render(vke::CommandBuffer& cmd) override;

    void set_camera(Camera* camera) { m_camera = camera; }

    MaterialID create_material(const std::string& pipeline_name, std::vector<ImageID> images = {}, const std::string& material_name = "");
    MeshID create_mesh(Mesh mesh, const std::string& name = "");
    RenderModelID create_model(MeshID mesh, MaterialID material, const std::string& name = "");
    RenderModelID create_model(const std::vector<std::pair<MeshID, MaterialID>>& parts, const std::string& name = "");
    ImageID create_image(std::unique_ptr<IImageView> image_view, const std::string& name = "");

    void bind_name2model(RenderModelID id, const std::string& name);

    RenderModelID get_model_id(const std::string& name) const { return m_render_model_names2model_ids.at(name); }

    std::optional<RenderModelID> try_get_model_id(const std::string& name) const { return at(m_render_model_names2model_ids, name); }
    std::optional<MeshID> try_get_mesh_id(const std::string& name) const { return at(m_mesh_names2mesh_ids, name); }
    std::optional<MaterialID> try_get_material_id(const std::string& name) const { return at(m_material_names2material_ids, name); }

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

    struct FramelyData {
        std::unique_ptr<vke::Buffer> light_buffer;
        VkDescriptorSet scene_light_set;
    };

private:
    uint32_t new_raw_id() { return m_id_counter++; }

    bool bind_mesh(RenderState& state, MeshID id);
    bool bind_material(RenderState& state, MaterialID id);

    void create_null_texture(int size);

    IImageView* get_image(ImageID id);

    RCResource<vke::IPipeline> load_pipeline_cached(const std::string& name);

    FramelyData& get_framely();

private: // rendering
    void update_lights();

private:
    FramelyData m_framely_data[FRAME_OVERLAP];
    VkDescriptorSetLayout m_scene_light_set_layout;
    std::unique_ptr<SceneSet> m_scene_set;

private:
    std::unordered_map<ImageID, std::unique_ptr<IImageView>> m_images;
    std::unordered_map<MaterialID, Material> m_materials;
    std::unordered_map<RenderModelID, RenderModel> m_render_models;
    std::unordered_map<MeshID, Mesh> m_meshes;

    std::unordered_map<std::string, MaterialID> m_material_names2material_ids;
    std::unordered_map<std::string, MeshID> m_mesh_names2mesh_ids;
    std::unordered_map<std::string, RenderModelID> m_render_model_names2model_ids;
    std::unordered_map<std::string, ImageID> m_image_names2image_ids;

    std::unordered_map<std::string, vke::RCResource<vke::IPipeline>> m_cached_pipelines;

    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;
    VkDescriptorSetLayout m_material_set_layout;
    VkSampler m_nearest_sampler;

    vke::RenderServer* m_render_server = nullptr;
    entt::registry* m_registry        = nullptr;
    Camera* m_camera                   = nullptr;
    IImageView* m_null_texture         = nullptr;

    ImageID m_null_texture_id;

    uint32_t m_id_counter = 1; // 0 is null
};

} // namespace vke