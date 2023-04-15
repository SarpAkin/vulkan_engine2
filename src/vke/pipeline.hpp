#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
#include "vk_resource.hpp"

namespace vke {

class GPipelineBuilder;

class Pipeline : public Resource {
public:
    friend GPipelineBuilder;
    Pipeline(Core* core, VkPipeline pipeline, VkPipelineLayout layout, VkPipelineBindPoint bindpoint) : Resource(core) {
        m_pipeline  = pipeline;
        m_layout    = layout;
        m_bindpoint = bindpoint;
    }
    ~Pipeline();

    VkPipeline handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_layout; }
    VkPipelineBindPoint bindpoint() const { return m_bindpoint; }

private:
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
    VkPipelineBindPoint m_bindpoint;
    std::vector<VkDescriptorSetLayout> m_dset_layouts;
};

} // namespace vke