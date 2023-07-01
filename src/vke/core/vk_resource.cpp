#include "vk_resource.hpp"

#include "core.hpp"

namespace vke {

VkDevice Resource::device() const {
    return core()->device();
}

Resource::Resource(Core* _core) : m_core(_core) {
#ifndef NDEBUG
    core()->resource_counter++;
#endif
    ;
}

Resource::~Resource() {
#ifndef NDEBUG
    core()->resource_counter--;
#endif


}
} // namespace vke