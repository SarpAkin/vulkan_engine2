#include "semaphore.hpp"

#include "core.hpp"
#include "vkutil.hpp"
#include <memory>

namespace vke {

Semaphore::Semaphore(Core* core) : Resource(core) {
    VkSemaphoreCreateInfo semaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VK_CHECK(vkCreateSemaphore(core->device(), &semaphoreCreateInfo, nullptr, &m_semaphore));
}

std::unique_ptr<Semaphore> Core::create_semaphore() {
    return std::make_unique<Semaphore>(this);
}

Semaphore::~Semaphore() {
    vkDestroySemaphore(core()->device(), m_semaphore, nullptr);
}

void Semaphore::reset() {
}

} // namespace vke
