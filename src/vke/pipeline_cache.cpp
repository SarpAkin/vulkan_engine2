#include "pipeline_cache.hpp"

#include "vkutil.hpp"

namespace vke {

PipelineCache::PipelineCache(Core* core, const char* cache_path) : Resource(core) {
    VkPipelineCacheCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
    };

    VK_CHECK(vkCreatePipelineCache(device(), &info, nullptr, &m_cache));
}

PipelineCache::~PipelineCache() {
    vkDestroyPipelineCache(device(), m_cache, nullptr);
}

} // namespace vke