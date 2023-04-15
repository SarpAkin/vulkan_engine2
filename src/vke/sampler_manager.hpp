#pragma once

#include <vector>

#include "fwd.hpp"
#include "vk_resource.hpp"

namespace vke {
class SamplerManager : private Resource {
private:
    friend Core;
    SamplerManager(Core*);

public:
    ~SamplerManager();

    VkSampler default_sampler() const { return m_default_sampler; }
    VkSampler nearest_sampler() const { return m_nearest_sampler; }

    VkSampler custom_sampler(const VkSamplerCreateInfo& sampler_info);

private:
    VkSampler m_default_sampler, m_nearest_sampler;

    std::vector<VkSampler> m_samplers;
};

} // namespace vke