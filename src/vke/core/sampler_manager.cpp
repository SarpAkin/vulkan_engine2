#include "sampler_manager.hpp"

#include "vkutil.hpp"

namespace vke {

VkSampler SamplerManager::custom_sampler(const VkSamplerCreateInfo& sampler_info) {
    VkSampler sampler;
    VK_CHECK(vkCreateSampler(device(), &sampler_info, nullptr, &sampler));
    m_samplers.push_back(sampler);

    return sampler;
}

SamplerManager::SamplerManager(Core* core) : Resource(core) {
    VkSamplerCreateInfo info{
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_NEAREST,
        .minFilter    = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    };

    m_nearest_sampler = custom_sampler(info);
    m_default_sampler = m_nearest_sampler;
}

SamplerManager::~SamplerManager() {
    for (auto& sampler : m_samplers) {
        vkDestroySampler(device(), sampler, nullptr);
    }
}

VkSampler SamplerManager::mipmap_nearest_sampler(int level) {
    return custom_sampler(VkSamplerCreateInfo{
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_NEAREST,
        .minFilter    = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .minLod = 0.0,
        .maxLod = static_cast<float>(level),
    });
}
} // namespace vke