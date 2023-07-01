#include "stencil_buffer.hpp"

#include "engine.hpp"

namespace vke {
std::unique_ptr<IStencilBuffer> IStencilBuffer::create_default_stencil_buffer(RenderEngine* engine) {
    return std::make_unique<StencilBuffer>(engine);
}

StencilBuffer::StencilBuffer(RenderEngine* engine) : System<StencilBufferFramelyData>(engine) {
    init_frame_datas([&](u32) {
        return StencilBufferFramelyData{};
    });
}

BufferSpan StencilBuffer::allocate(usize size) {
    auto& data = get_frame_data();
    if (data.last_frame != engine()->get_frames_since_start()) 
        reset_stencil();

    if (data.stencils.empty()) new_stencil();

    while (true) {
        auto& stencil = data.stencils.back();
        if (stencil.top + size <= stencil.buffer->byte_size()) {
            auto span = stencil.buffer->subspan(stencil.top, size);
            stencil.top += size;
            return span;
        }

        new_stencil();
    }
}

void StencilBuffer::reset_stencil() {
    auto& data      = get_frame_data();
    data.last_frame = engine()->get_frames_since_start();

    for (auto& stencil : data.stencils) {
        m_free_buffers.push_back(std::move(stencil.buffer));
    }

    data.stencils.clear();
}

void StencilBuffer::new_stencil() {
    auto& data = get_frame_data();

    if (m_free_buffers.size()) {
        data.stencils.push_back(StencilBufferFramelyData::Stencil{
            .buffer = std::move(m_free_buffers.back()),
            .top    = 0,
        });
        m_free_buffers.pop_back();
    } else {
        data.stencils.push_back(StencilBufferFramelyData::Stencil{
            .buffer = core()->create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 1 << 20, true),
            .top    = 0,
        });
    }
};

} // namespace vke