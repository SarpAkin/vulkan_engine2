#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
#include "vk_resource.hpp"

namespace vke {

class GPipelineBuilder;
class CPipelineBuilder;

class Pipeline : public Resource {
    friend GPipelineBuilder;
    friend CPipelineBuilder;

public:
    struct PipelineData {
        VkShaderStageFlagBits push_stages;
        std::vector<VkDescriptorSetLayout> dset_layouts;
    };

    Pipeline(Core* core, VkPipeline pipeline, VkPipelineLayout layout, VkPipelineBindPoint bindpoint) : Resource(core) {
        m_pipeline  = pipeline;
        m_layout    = layout;
        m_bindpoint = bindpoint;
    }
    ~Pipeline();

    VkPipeline handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_layout; }
    VkPipelineBindPoint bindpoint() const { return m_bindpoint; }

    const PipelineData& data() const { return m_data; }
    VkDescriptorSetLayout get_descriptor_layout(u32 index) const { return m_data.dset_layouts[index]; }

private:
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
    VkPipelineBindPoint m_bindpoint;
    PipelineData m_data;
};

} // namespace vke