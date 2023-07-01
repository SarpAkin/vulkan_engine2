#pragma once

#include "../common.hpp"
#include "../fwd.hpp"
#include "vk_resource.hpp"

namespace vke {
class PipelineCache : public Resource {
public:
    PipelineCache(Core* core,const char* cache_path = nullptr);
    ~PipelineCache();

    VkPipelineCache handle() { return m_cache; }

private:
    VkPipelineCache m_cache;
};


} // namespace vke