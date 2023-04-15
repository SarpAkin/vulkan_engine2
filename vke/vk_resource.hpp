#pragma once

#include "common.hpp"
#include "fwd.hpp"

#include <vulkan/vulkan_core.h>

namespace vke {

class Resource {
public:
    inline Core* core() const { return m_core; }
    VkDevice device() const;

    Resource(Core* core) : m_core(core) {}
    virtual ~Resource(){};

    Resource(const Resource&)            = delete;
    Resource(Resource&&)                 = delete;
    Resource& operator=(const Resource&) = delete;
    Resource& operator=(Resource&&)      = delete;

protected:
private:
    Core* m_core;
};

} // namespace vke