#include "pipeline.hpp"
#include "pipeline_reflections.hpp"
#include <vulkan/vulkan_core.h>

namespace vke {

Pipeline::Pipeline(Core* core, VkPipeline pipeline, VkPipelineLayout layout, VkPipelineBindPoint bindpoint) : Resource(core) {
    m_pipeline  = pipeline;
    m_layout    = layout;
    m_bindpoint = bindpoint;
}

Pipeline::~Pipeline() {
    vkDestroyPipeline(device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(device(), m_layout, nullptr);
}

} // namespace vke
