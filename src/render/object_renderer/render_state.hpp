#pragma once

#include "resource_manager.hpp"

namespace vke {

struct RenderTargetInfo {
    std::string subpass_name;
    SetIndices set_indices;
    VkDescriptorSet view_sets[2];
    vke::Camera* camera;
};

} // namespace vke