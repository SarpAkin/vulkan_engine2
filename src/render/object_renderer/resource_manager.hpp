#pragma once

#include "render/mesh/mesh.hpp"

#include "common.hpp"

#include "../iobject_renderer.hpp"
#include "render/object_renderer/renderer_common.hpp"
#include <unordered_map>

#include <vke/vke.hpp>

namespace vke {

struct RenderState;
struct RenderTargetInfo;

class ResourceManager : vke::DeviceGetter {
public:
    struct MultiPipeline;
    struct Material;
    struct RenderModel;
    struct UpdatedResources;

    struct BindState {
        vke::CommandBuffer& cmd;
        MaterialID bound_material_id              = 0;
        MeshID bound_mesh_id                      = 0;
        IPipeline* bound_pipeline                 = nullptr;
        const Mesh* mesh                          = nullptr;
        const ResourceManager::Material* material = nullptr;
        const RenderTargetInfo* rd_info                 = nullptr;
    };

public:
    ResourceManager(RenderServer* render_server);
    ~ResourceManager();

public: // getters
    VkSampler get_nearest_sampler() { return m_nearest_sampler; }
    IImageView* get_null_texture() { return m_null_texture; }
    VkDescriptorSetLayout get_material_set_layout() const { return m_material_set_layout; }
    UpdatedResources& get_updated_resource() { return m_updates; }
    // id getters
    RenderModelID get_model_id(const std::string& name) const { return m_render_model_names2model_ids.at(name); }

    std::optional<RenderModelID> try_get_model_id(const std::string& name) const { return vke::at(m_render_model_names2model_ids, name); }
    std::optional<MeshID> try_get_mesh_id(const std::string& name) const { return vke::at(m_mesh_names2mesh_ids, name); }
    std::optional<MaterialID> try_get_material_id(const std::string& name) const { return vke::at(m_material_names2material_ids, name); }
    std::optional<ImageID> try_get_image_id(const std::string& name) const { return vke::at(m_image_names2image_ids, name); }

    // resource getters
    IImageView* get_image(ImageID id);
    IImageView* get_image(const std::string& name) {
        return vke::map_optional(try_get_image_id(name), [&](auto id) { return get_image(id); }).value_or(nullptr);
    }

    const RenderModel* get_model(RenderModelID id) const { return vke::at_ptr(m_render_models, id); }
    const RenderModel* get_model(const std::string& name) const {
        return vke::map_optional(try_get_model_id(name), [&](auto id) { return get_model(id); }).value_or(nullptr);
    }

    const Material* get_material(MaterialID id) const { return vke::at_ptr(m_materials, id); }
    const Material* get_material(const std::string& name) const {
        return vke::map_optional(try_get_material_id(name), [&](auto id) { return get_material(id); }).value_or(nullptr);
    }

    const Mesh* get_mesh(MeshID id) const { return vke::at_ptr(m_meshes, id); }
    const Mesh* get_mesh(const std::string& name) const {
        return vke::map_optional(try_get_mesh_id(name), [&](auto id) { return get_mesh(id); }).value_or(nullptr);
    }

    // subpass type getter/setter
    void set_subpass_type(const std::string& subpass_name, MaterialSubpassType type) { m_subpass_types[subpass_name] = type; }
    MaterialSubpassType get_subpass_type(const std::string& name) const { return vke::at(m_subpass_types, name).value_or(MaterialSubpassType::NONE); }

public: // creation
    void create_multi_target_pipeline(const std::string& name, std::span<const std::string> pipelines);
    void add_pipeline2multi_pipeline(const std::string& multi_pipeline_name, const std::string& pipeline_name, const std::string& renderpass_name = "", std::span<const std::string> modifiers = {});

    MaterialID create_material(const std::string& multi_pipeline_name, std::vector<ImageID> images = {}, const std::string& material_name = "");
    MeshID create_mesh(Mesh mesh, const std::string& name = "");
    RenderModelID create_model(MeshID mesh, MaterialID material, const std::string& name = "");
    RenderModelID create_model(const std::vector<std::pair<MeshID, MaterialID>>& parts, const std::string& name = "");
    ImageID create_image(std::unique_ptr<IImageView> image_view, const std::string& name = "");

    void bind_name2model(RenderModelID id, const std::string& name);

public: // render state binding
    BindState create_bindstate(vke::CommandBuffer& cmd,const RenderTargetInfo* target_info);

    bool bind_mesh(BindState* state, MeshID id);
    bool bind_material(BindState* state, MaterialID id);

private:
    void calculate_boundary(RenderModel& model);
    void create_null_texture(int size);

    RCResource<vke::IPipeline> load_pipeline_cached(const std::string& name);

public:
    struct RenderModel {
        struct Part {
            MeshID mesh_id;
            MaterialID material_id;
        };

        vke::SmallVec<Part> parts;
        std::string name;
        AABB boundary;
    };

    struct MultiPipeline {
        std::unordered_map<std::string, vke::RCResource<IPipeline>> pipelines;
        std::string name;
    };

    struct Material {
        MultiPipeline* multi_pipeline;
        VkDescriptorSet material_set;
        vke::SmallVec<ImageID> images;
        std::string name;
    };

    struct UpdatedResources {
        vke::SlimVec<ImageID> image_updates;
        vke::SlimVec<MaterialID> material_updates;
        vke::SlimVec<RenderModelID> model_updates;
        vke::SlimVec<MeshID> mesh_updates;

        void reset() {
            image_updates.clear();
            material_updates.clear();
            model_updates.clear();
            mesh_updates.clear();
        }
    };

private:
    std::unordered_map<ImageID, std::unique_ptr<IImageView>> m_images;
    std::unordered_map<MaterialID, Material> m_materials;
    std::unordered_map<RenderModelID, RenderModel> m_render_models;
    std::unordered_map<MeshID, Mesh> m_meshes;

    GenericIDManager<ImageID> m_image_id_manager;
    GenericIDManager<MaterialID> m_material_id_manager;
    GenericIDManager<RenderModelID> m_render_model_id_manager;
    GenericIDManager<MeshID> m_mesh_id_manager;

    std::unordered_map<std::string, MaterialID> m_material_names2material_ids;
    std::unordered_map<std::string, MeshID> m_mesh_names2mesh_ids;
    std::unordered_map<std::string, RenderModelID> m_render_model_names2model_ids;
    std::unordered_map<std::string, ImageID> m_image_names2image_ids;

    std::unordered_map<std::string, vke::RCResource<vke::IPipeline>> m_cached_pipelines;

    // pointer stability is required since there are pointers to values of this container
    std::unordered_map<std::string, MultiPipeline> m_multi_pipelines;

    std::unordered_map<std::string, MaterialSubpassType> m_subpass_types;

    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;

    VkDescriptorSetLayout m_material_set_layout;

    VkSampler m_nearest_sampler;

    IImageView* m_null_texture = nullptr;
    ImageID m_null_texture_id;

    vke::RenderServer* m_render_server;

    UpdatedResources m_updates;
};

} // namespace vke