#pragma once

#include <cassert>

#include <fmt/format.h>

#include <span>
#include <vulkan/vulkan_core.h>

#include "../common.hpp"
#include "../fwd.hpp"

namespace vke {
const char* vk_result_string(VkResult res);

bool is_depth_format(VkFormat format);

struct ShaderCompileOptions {
    std::span<const std::pair<std::string, std::string>> defines;
};

std::span<u32> compile_glsl_file(ArenaAllocator* alloc, const char* path, ShaderCompileOptions* options = nullptr);

} // namespace vke

#define VK_CHECK(x)                                                                    \
    {                                                                                  \
        VkResult result = x;                                                           \
        if (result != VK_SUCCESS) {                                                    \
            fmt::print(stderr, "[Vulkan Error]: {}\n", vke::vk_result_string(result)); \
            assert(0);                                                                 \
        }                                                                              \
    }
