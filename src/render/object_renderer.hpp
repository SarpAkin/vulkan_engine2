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
    constexpr static int VIEW_SET     = 1;
    constexpr static int MATERIAL_SET = 2;
    constexpr static int LIGHT_SET    = -1;

    constexpr static int MATERIAL_SET_IMAGE_COUNT = 4;

    enum class MaterialSubpassType {
        NONE = 0,
        FORWARD,
        DEFERRED_PBR,
        SHADOW,
        CUSTOM,
    };

public:
    ObjectRenderer(RenderServer* render_server);
    ~ObjectRenderer();

    constexpr static const char* pbr_pipeline_name = "vke::object_renderer::pbr_pipeline";
    void create_default_pbr_pipeline();

    void set_entt_registry(entt::registry* registry) override { m_registry = registry; }
    void render(const RenderArguments& args) override;

    void set_camera(const std::string& render_target, Camera* camera) { m_render_targets.at(render_target).camera = camera; }

    void set_subpass_type(const std::string& subpass_name, MaterialSubpassType type) { m_subpass_types[subpass_name] = type; }
    MaterialSubpassType get_subpass_type(const std::string& name) const { return vke::at(m_subpass_types, name).value_or(MaterialSubpassType::NONE); }

    void create_render_target(const std::string& name, const std::string& subpass_name);

    void create_multi_target_pipeline(const std::string& name, std::span<const std::string> pipelines);

    MaterialID create_material(const std::string& multi_pipeline_name, std::vector<ImageID> images = {}, const std::string& material_name = "");
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

    struct MultiPipeline {
        RCResource<IPipeline> deferred_pipeline;
        RCResource<IPipeline> forward_pipeline;
        RCResource<IPipeline> shadow_pipeline;
    };

    struct Material {
        MultiPipeline* multi_pipeline;
        VkDescriptorSet material_set;
        vke::SmallVec<ImageID> images;
        std::string name;
    };

    struct RenderTarget {
        std::string renderpass_name;
        MaterialSubpassType subpass_type = MaterialSubpassType::NONE;
        vke::Camera* camera              = nullptr;

        VkDescriptorSet view_sets[FRAME_OVERLAP];
        std::unique_ptr<vke::Buffer> view_buffers[FRAME_OVERLAP];
    };

    struct RenderState {
        vke::CommandBuffer& cmd;
        vke::CommandBuffer& compute_cmd;
        MaterialID bound_material_id = 0;
        MeshID bound_mesh_id         = 0;
        IPipeline* bound_pipeline    = nullptr;
        Mesh* mesh                   = nullptr;
        Material* material           = nullptr;
        RenderTarget* render_target  = nullptr;
    };

    struct FramelyData {
        std::unique_ptr<vke::Buffer> light_buffer;
        VkDescriptorSet scene_set;
    };

private:
    uint32_t new_raw_id() { return m_id_counter++; }

    bool bind_mesh(RenderState& state, MeshID id);
    bool bind_material(RenderState& state, MaterialID id);

    void create_null_texture(int size);

    IImageView* get_image(ImageID id);

    RCResource<vke::IPipeline> load_pipeline_cached(const std::string& name);

    FramelyData& get_framely();

    static IPipeline* get_pipeline(MultiPipeline* mp, RenderTarget* target);

    void update_view_set(RenderTarget* target);


private: // rendering
    void update_lights();

private:
    FramelyData m_framely_data[FRAME_OVERLAP];
    VkDescriptorSetLayout m_scene_set_layout, m_view_set_layout;

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

    std::unordered_map<std::string, RenderTarget> m_render_targets;
    std::unordered_map<std::string, MaterialSubpassType> m_subpass_types;
    // pointer stability is required since there are pointers to values of this container
    std::unordered_map<std::string, MultiPipeline> m_multi_pipelines;

    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;
    VkDescriptorSetLayout m_material_set_layout;
    VkSampler m_nearest_sampler;

    vke::RenderServer* m_render_server = nullptr;
    entt::registry* m_registry         = nullptr;
    IImageView* m_null_texture         = nullptr;

    ImageID m_null_texture_id;

    uint32_t m_id_counter = 1; // 0 is null
};

} // namespace vke