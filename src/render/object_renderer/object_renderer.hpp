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

    void set_entt_registry(entt::registry* registry) override { m_registry = registry; }
    void render(const RenderArguments& args) override;

    void set_camera(const std::string& render_target, Camera* camera) { m_render_targets.at(render_target).camera = camera; }

    ResourceManager* get_resource_manager() { return m_resource_manager.get(); }

    void create_render_target(const std::string& name, const std::string& subpass_name);

public:
    struct RenderTarget {
        std::string renderpass_name;
        MaterialSubpassType subpass_type = MaterialSubpassType::NONE;
        vke::Camera* camera              = nullptr;

        VkDescriptorSet view_sets[FRAME_OVERLAP];
        std::unique_ptr<vke::Buffer> view_buffers[FRAME_OVERLAP];
    };

private:
    struct FramelyData {
        std::unique_ptr<vke::Buffer> light_buffer;
        VkDescriptorSet scene_set;
    };

private:
    FramelyData& get_framely();

private: // rendering
    void update_view_set(RenderTarget* target);
    void update_lights();

private:
    FramelyData m_framely_data[FRAME_OVERLAP];
    VkDescriptorSetLayout m_scene_set_layout, m_view_set_layout;

private:
    std::unordered_map<std::string, RenderTarget> m_render_targets;

    std::unique_ptr<vke::DescriptorPool> m_descriptor_pool;
    std::unique_ptr<ResourceManager> m_resource_manager;

    vke::RenderServer* m_render_server = nullptr;
    entt::registry* m_registry         = nullptr;
};

} // namespace vke