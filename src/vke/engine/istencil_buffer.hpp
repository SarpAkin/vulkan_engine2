#pragma once

#include <memory>
#include <vector>

#include <cstring>

#include "../core/buffer.hpp"
#include "../fwd.hpp"

namespace vke {

class IStencilBuffer {
public:
    virtual ~IStencilBuffer(){};

    virtual BufferSpan allocate(usize size) = 0;

    template <typename T>
    BufferSpan upload(std::span<const T> data) {
        BufferSpan bspan = allocate(data.size_bytes());
        memcpy(bspan.mapped_data_bytes().data(), data.data(), data.size_bytes());
        return bspan;
    }

    // stencil_buffer.cpp
    static std::unique_ptr<IStencilBuffer> create_default_stencil_buffer(RenderEngine* engine);
};

} // namespace vke