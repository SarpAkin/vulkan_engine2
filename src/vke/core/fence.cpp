#include "fence.hpp"

#include <memory>
#include <vulkan/vulkan_core.h>

#include "commandbuffer.hpp"
#include "core.hpp"
#include "vkutil.hpp"

#include "../fwd.hpp"
#include "../util.hpp"

namespace vke {

Fence::Fence(Core* core, bool signaled) : Resource(core) {
    VkFenceCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : (VkFenceCreateFlagBits)0,
    };

    VK_CHECK(vkCreateFence(device(), &info, nullptr, &m_fence));
}

std::unique_ptr<Fence> Core::create_fence(bool signaled) {
    return std::make_unique<Fence>(this, signaled);
}

Fence::~Fence() {
    vkDestroyFence(device(), m_fence, nullptr);
}

void Fence::reset() {
    VK_CHECK(vkResetFences(device(), 1, &m_fence));
}

void Fence::wait(u64 timeout) {
    VK_CHECK(vkWaitForFences(device(), 1, &m_fence, true, timeout));
}

void Fence::submit(VkSubmitInfo* info, std::span<CommandBuffer*> cmds) {
    VK_CHECK(vkQueueSubmit(core()->queue(), 1, info, handle()));

    m_execution_dependencies = map_vec(cmds, [](CommandBuffer* cmd) {
        return std::move(cmd->m_dependent_resources);
    });
}

} // namespace vke