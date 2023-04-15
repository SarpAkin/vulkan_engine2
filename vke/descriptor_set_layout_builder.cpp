#include "descriptor_set_layout_builder.hpp"

#include <vulkan/vulkan_core.h>

#include "core.hpp"
#include "vkutil.hpp"

namespace vke {

void DescriptorSetLayoutBuilder::add_binding(VkDescriptorType type, VkShaderStageFlags stage, uint32_t count) {
    m_bindings.push_back(VkDescriptorSetLayoutBinding{
        .binding         = static_cast<uint32_t>(m_bindings.size()),
        .descriptorType  = type,
        .descriptorCount = count,
        .stageFlags      = stage,
    });
}

VkDescriptorSetLayout DescriptorSetLayoutBuilder::build(Core* core) {
    VkDescriptorSetLayoutCreateInfo info{
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(m_bindings.size()),
        .pBindings    = m_bindings.data(),
    };

    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(core->device(), &info, nullptr, &layout));

    core->queue_destroy(layout);

    return layout;
}

} // namespace vke
