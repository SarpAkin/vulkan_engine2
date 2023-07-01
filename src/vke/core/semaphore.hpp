#pragma once

#include "../common.hpp"
#include "../fwd.hpp"
#include "vk_resource.hpp"

#include <vulkan/vulkan_core.h>

namespace vke {

class Semaphore : public Resource {
public:
    const VkSemaphore& handle() const { return m_semaphore; }
    void reset();


    Semaphore(Core* core);
    ~Semaphore();

private:
    VkSemaphore m_semaphore = nullptr;
};

} // namespace vke
