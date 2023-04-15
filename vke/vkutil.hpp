#pragma once

#include <cassert>

#include <fmt/format.h>

#include <vulkan/vulkan_core.h>

namespace vke {
const char* vk_result_string(VkResult res);

bool is_depth_format(VkFormat format);

} // namespace vke

#define VK_CHECK(x)                                                                    \
    {                                                                                  \
        VkResult result = x;                                                           \
        if (result != VK_SUCCESS) {                                                    \
            fmt::print(stderr, "[Vulkan Error]: {}\n", vke::vk_result_string(result)); \
            assert(0);                                                                 \
        }                                                                              \
    }
