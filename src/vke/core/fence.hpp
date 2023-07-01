#pragma once

#include "vk_resource.hpp"
#include "../fwd.hpp"
#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace vke {

class Fence : public Resource {
public:
    Fence(Core* core, bool signaled = false);
    virtual ~Fence();

    const VkFence& handle() const { return m_fence; }

    void reset();
    void wait(u64 timeout = UINT64_MAX);

    void submit(VkSubmitInfo* submit_info,std::span<CommandBuffer*> cmds);
    
private:
    VkFence m_fence = nullptr;
    std::vector<std::vector<std::unique_ptr<Resource>>> m_execution_dependencies;
};

} // namespace vke
