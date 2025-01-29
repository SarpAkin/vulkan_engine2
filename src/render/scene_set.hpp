#pragma once

#include <vke/fwd.hpp>

#include <memory>
#include <vulkan/vulkan.h>

#include "../fwd.hpp"
#include "common.hpp"

namespace vke {

class SceneSet {
public:
    SceneSet(RenderServer* engine);
    ~SceneSet();

    Camera* camera;
    Scene* scene;

public:
    void update_scene_set();
    VkDescriptorSet get_scene_set() const;
    VkDescriptorSetLayout get_scene_set_layout() const { return m_descriptor_set_layout; }

private:
    struct FramelyData {
        std::unique_ptr<vke::Buffer> scene_ubo;
        VkDescriptorSet set = nullptr;
    };

private:
    FramelyData m_framely_datas[FRAME_OVERLAP];
    RenderServer* m_render_server;
    VkDescriptorSetLayout m_descriptor_set_layout = nullptr;
};

} // namespace vke