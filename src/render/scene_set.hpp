#pragma once

#include <vke/fwd.hpp>

#include <memory>
#include <vulkan/vulkan.h>

#include "../fwd.hpp"

namespace vke {

class SceneData {
public:
    SceneData(RenderServer* engine);
    ~SceneData();

    Camera* camera;

public:
    VkDescriptorSet get_scene_set() const { return m_descriptor_set; }
    VkDescriptorSetLayout get_scene_set_layout() const { return m_descriptor_set_layout; }

private:
    RenderServer* m_engine;
    std::unique_ptr<vke::Buffer> m_scene_ubo;
    VkDescriptorSet m_descriptor_set              = nullptr;
    VkDescriptorSetLayout m_descriptor_set_layout = nullptr;
};

} // namespace vke