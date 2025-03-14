#pragma once

#include <unordered_map>
#include <vector>

#include <vke/util.hpp>
#include <vke/vke.hpp>

#include "common.hpp"
#include "render/iobject_renderer.hpp"
#include "render/mesh/mesh.hpp"
#include "renderer_common.hpp"

#include "fwd.hpp"

namespace vke {

class ResourceManager;

struct RenderState;

class ObjectRenderer final : public IObjectRenderer, DeviceGetter {
public:
    constexpr static int SCENE_SET    = 0;
    constexpr static int VIEW_SET     = 1;
    constexpr static int MATERIAL_SET = 2;
    constexpr static int LIGHT_SET    = -1;

    constexpr static int MATERIAL_SET_IMAGE_COUNT = 4;

public:
public:
    ObjectRenderer(RenderServer* render_server);
    ~ObjectRenderer();

    constexpr static const char* pbr_pipeline_name = "vke::object_renderer::pbr_pipeline";
    void create_default_pbr_pipeline();

    void set_entt_registry(entt::registry* registry) override;
    void render(const RenderArguments& args) override;

    void set_camera(const std::string& render_target, Camera* camera) { m_render_targets.at(render_target).camera = camera; }

    ResourceManager* get_resource_manager() { return m_resource_manager.get(); }

    void create_render_target(const std::string& name, const std::string& subpass_name,bool allow_indirect_render = false);

private:
    struct IndirectRenderBuffers;

public:
    struct RenderTarget {
        std::string renderpass_name;
        MaterialSubpassType subpass_type = MaterialSubpassType::NONE;
        vke::Camera* camera              = nullptr;

        VkDescriptorSet view_sets[FRAME_OVERLAP];
        std::unique_ptr<vke::Buffer> view_buffers[FRAME_OVERLAP];
        // if this is null the scene should be renderer using render_direct
        // otherwise it can be rendered using render_indirect
        std::unique_ptr<IndirectRenderBuffers> indirect_render_buffers;
    };

    using InstanceID = impl::GenericID<struct ObjectInstance>;

private:
    struct FramelyData {
        std::unique_ptr<vke::Buffer> light_buffer;
        VkDescriptorSet scene_set;
    };

    struct IndirectRenderSceneData {
        std::unique_ptr<vke::Buffer> model_info_buffer;
        std::unique_ptr<vke::Buffer> model_part_info_buffer;
        // allocates sub ranges from model_part_info_buffer.
        // allocations are done in parts not bytes
        VirtualAllocator model_part_buffer_sub_allocator;
        std::unordered_map<RenderModelID, VirtualAllocator::Allocation> model_part_sub_allocations;

        std::unique_ptr<vke::Buffer> material_info_buffer;

        // stores instance specific data
        std::unique_ptr<vke::GrowableBuffer> instance_buffer;

        std::unique_ptr<vke::Buffer> mesh_info_buffer;

        GenericIDManager<InstanceID> instance_id_manager = GenericIDManager<InstanceID>(0);
        vke::SlimVec<entt::entity> pending_entities_for_register;
        vke::SlimVec<InstanceID> pending_instances_for_destruction;

        RCResource<vke::IPipeline> cull_pipeline;
        RCResource<vke::IPipeline> indirect_draw_command_gen_pipeline;

        std::unordered_map<RenderModelID, i32> model_instance_counters;
    };

    struct IndirectRenderBuffers {
        std::unique_ptr<vke::Buffer> indirect_draw_buffer;
        // stores indirect draw commands

        // this buffer stores indexes to parts indirect draw arguments
        std::unique_ptr<vke::Buffer> part2indirect_draw_location[FRAME_OVERLAP];

        // it is a an array of uvec2. their indices correspond to their part id
        // it stores draw offsets & draw max_counts in x & y components respectively
        std::unique_ptr<vke::Buffer> instance_draw_parameter_location_buffer[FRAME_OVERLAP];
        std::unique_ptr<vke::Buffer> instance_count_buffer;

        std::unique_ptr<vke::GrowableBuffer> instance_draw_parameters;
    };

private:
    FramelyData& get_framely();

    void render_direct(RenderState& state);
    void render_indirect(RenderState& state);

private: // rendering
    void update_view_set(RenderTarget* target);
    void update_lights();

private: // indirect render
    void connect_registry_callbacks();
    void disconnect_registry_callbacks();

    void renderable_component_creation_callback(entt::registry&, entt::entity e);
    void renderable_component_update_callback(entt::registry&, entt::entity e);
    void renderable_component_destroy_callback(entt::registry&, entt::entity e);

    void updates_for_indirect_render(vke::CommandBuffer& compute_cmd);

    void flush_pending_entities(vke::CommandBuffer& cmd, StencilBuffer& stencil);

    void initialize_scene_data();

private:
    FramelyData m_framely_data[FRAME_OVERLAP];
    VkDescriptorSetLayout m_scene_set_layout, m_view_set_layout;

private:
    std::unordered_map<std::string, RenderTarget> m_render_targets;

    std::unique_ptr<vke::Buffer> m_dummy_buffer; // 256 byte long buffer that is designed to fill descriptor binding 

    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;
    std::unique_ptr<ResourceManager> m_resource_manager;

    // gpu side scene data meant for indirect render
    std::unique_ptr<IndirectRenderSceneData> m_scene_data;

    vke::RenderServer* m_render_server = nullptr;
    entt::registry* m_registry         = nullptr;
};

} // namespace vke