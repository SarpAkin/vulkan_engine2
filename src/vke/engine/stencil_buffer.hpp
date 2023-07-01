#pragma once

#include "istencil_buffer.hpp"
#include "vk_system.hpp"

#include "../core/buffer.hpp"

namespace vke {

struct StencilBufferFramelyData {
    struct Stencil {
        std::unique_ptr<vke::Buffer> buffer;
        usize top = 0;
    };

    std::vector<Stencil> stencils;
    usize last_frame = -1;
};

class StencilBuffer : public IStencilBuffer, System<StencilBufferFramelyData> {
public:
    StencilBuffer(RenderEngine* engine);
    ~StencilBuffer(){}

    BufferSpan allocate(usize size) override;
private:
    void reset_stencil();
    void new_stencil();

    std::vector<std::unique_ptr<vke::Buffer>> m_free_buffers;
};

} // namespace vke