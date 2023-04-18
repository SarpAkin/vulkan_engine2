#include "vk_system.hpp"

#include <cassert>

#include "core.hpp"
#include "engine.hpp"

namespace vke {

SystemBase::SystemBase(RenderEngine* engine) {
    assert(engine != nullptr);

    m_engine = engine;
    m_core   = engine->core();
}

VkDevice SystemBase::device() const {
    return core()->device();
}

u32 SystemBase::get_frame_index() const { return engine()->get_frame_index(); }
u32 SystemBase::get_frame_overlap() const { return engine()->get_frame_overlap(); }

DescriptorPool* SystemBase::get_framely_descriptor_pool() {
    return engine()->get_framely_pool();
}

} // namespace vke