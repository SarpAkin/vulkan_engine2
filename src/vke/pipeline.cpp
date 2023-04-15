#include "pipeline.hpp"
#include <vulkan/vulkan_core.h>

namespace vke {
Pipeline::~Pipeline() {
    vkDestroyPipeline(device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(device(), m_layout, nullptr);
}

} // namespace vke
