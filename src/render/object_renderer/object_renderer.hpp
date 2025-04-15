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
class SceneBuffersManager;
class LightBuffersManager;

struct RenderState;

class ObjectRenderer final : public IObjectRenderer, DeviceGetter {
public:
    constexpr static int MATERIAL_SET_IMAGE_COUNT = 4;

public:
public:
    ObjectRenderer(RenderServer* render_server);
    ~ObjectRenderer();

    constexpr static const char* pbr_pipeline_name = "vke::object_renderer::pbr_pipeline";
    void create_default_pbr_pipeline();

    void set_entt_registry(entt::registry* registry) override;
    void render(const RenderArguments& args) override;
    void update_scene_data(CommandBuffer& cmd);

    void set_camera(const std::string& render_target, Camera* camera) { m_render_targets.at(render_target).camera = camera; }

    ResourceManager* get_resource_manager() { return m_resource_manager.get(); }

    void create_render_target(const std::string& name, const std::string& subpass_name, bool allow_indirect_render = false);

    LightBuffersManager* get_light_manager() { return m_light_manager.get(); }
    IBuffer* get_view_buffer(const std::string& render_target_name) const;

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

        SetIndices set_indices;
    };

private:
    struct FramelyData {
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

private:
    void initialize_scene_data();

private:
    FramelyData m_framely_data[FRAME_OVERLAP];
    VkDescriptorSetLayout m_view_set_layout;

private:
    std::unordered_map<std::string, RenderTarget> m_render_targets;

    std::unique_ptr<vke::Buffer> m_dummy_buffer; // 256 byte long buffer that is designed to fill descriptor binding

    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;
    std::unique_ptr<ResourceManager> m_resource_manager;


    vke::RenderServer* m_render_server = nullptr;
    entt::registry* m_registry         = nullptr;

private://indirect render related data
    RCResource<vke::IPipeline> m_cull_pipeline;
    RCResource<vke::IPipeline> m_indirect_draw_command_gen_pipeline;

    std::unique_ptr<SceneBuffersManager> m_scene_data;
    std::unique_ptr<LightBuffersManager> m_light_manager;
};

} // namespace vke