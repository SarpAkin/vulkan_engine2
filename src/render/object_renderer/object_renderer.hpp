#pragma once

#include <unordered_map>
#include <vector>

#include <vke/util.hpp>
#include <vke/vke.hpp>

#include "common.hpp"
#include "iobject_renderer_system.hpp"
#include "render/iobject_renderer.hpp"
#include "render/mesh/mesh.hpp"
#include "render/object_renderer/render_state.hpp"
#include "renderer_common.hpp"

#include "fwd.hpp"

namespace vke {

class ResourceManager;
class SceneBuffersManager;
class LightBuffersManager;

struct RenderState;

struct RenderTargetArguments {
    bool allow_indirect_render = true;
    bool allow_hzb_culling     = false;
};

class ObjectRenderer final : public DeviceGetter {
public:
    constexpr static int MATERIAL_SET_IMAGE_COUNT = 4;

public:
public:
    ObjectRenderer(RenderServer* render_server);
    ~ObjectRenderer();

    constexpr static const char* pbr_pipeline_name = "vke::object_renderer::pbr_pipeline";
    void create_default_pbr_pipeline();

    RenderServer* get_render_server() const { return m_render_server; }

    void set_world(flecs::world* registry);
    void render(const RenderArguments& args);
    void update_scene_data(CommandBuffer& cmd);

    void set_camera(const std::string& render_target, Camera* camera);
    void set_hzb(const std::string& render_target, HierarchicalZBuffers* hzb);

    ResourceManager* get_resource_manager() { return m_resource_manager.get(); }

    void create_render_target(const std::string& name, const std::string& subpass_name, const RenderTargetArguments& render_target_arguments = {});

    LightBuffersManager* get_light_manager() { return m_light_manager.get(); }
    IBuffer* get_view_buffer(const std::string& render_target_name, int frame_index) const;

    void add_render_system(std::unique_ptr<IObjectRendererSystem> rs) { m_render_systems.push_back(std::move(rs)); }

    const RenderTargetInfo* get_render_target_info(const std::string& render_target_name) const { return &m_render_targets.at(render_target_name).info; }

private:
    struct IndirectRenderBuffers;

public:
    struct RenderTarget {
        RenderTargetInfo info;

        std::unique_ptr<vke::Buffer> view_buffers[FRAME_OVERLAP];
        bool is_view_set_needs_update[FRAME_OVERLAP];
        HierarchicalZBuffers* hzb;
    };

private:
    struct FramelyData {
    };

private:
    FramelyData& get_framely();

    void render_direct(RenderState& state);
    void render_indirect(RenderState& state);

private: // rendering
    void update_view_set(RenderTarget* target);

private:
    void create_render_systems();

    void update_view_descriptor_set(RenderTarget* rd, u32 frame_index);

    void debug_menu();

private:
    FramelyData m_framely_data[FRAME_OVERLAP];
    VkDescriptorSetLayout m_view_set_layout;

private:
    std::unordered_map<std::string, RenderTarget> m_render_targets;

    std::unique_ptr<vke::Buffer> m_dummy_buffer; // 256 byte long buffer that is designed to fill descriptor binding

    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;
    std::unique_ptr<ResourceManager> m_resource_manager;

    vke::RenderServer* m_render_server = nullptr;
    flecs::world* m_world         = nullptr;

    glm::dvec3 m_render_origin = {0, 0, 0};

    std::vector<std::unique_ptr<IObjectRendererSystem>> m_render_systems;

private:
    std::unique_ptr<LightBuffersManager> m_light_manager;

    bool m_query_indirect_render_counters = true;
};

} // namespace vke