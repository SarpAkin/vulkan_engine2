#include "vk_resource.hpp"

#include "core.hpp"

namespace vke
{

VkDevice Resource::device() const
{
    return core()->device();
}

} // namespace vke