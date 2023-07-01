#include "batch_uploader.hpp"

#include "istencil_buffer.hpp"

#include "../core/commandbuffer.hpp"

namespace vke {
BatchUploader::BatchUploader(IStencilBuffer* stencil, vke::CommandBuffer* cmd) {
    m_cmd     = cmd;
    m_stencil = stencil;

    m_copies.reserve(16);
}

void BatchUploader::upload(std::span<const u8> data, const IBufferSpan& buffer_span) {
    assert(buffer_span.byte_size() >= data.size_bytes());
    upload(data, buffer_span.handle(), buffer_span.byte_offset());
}

void BatchUploader::upload(std::span<const u8> data, VkBuffer target_buffer, usize offset) {
    auto stencil_upload = m_stencil->upload(data);

    if (stencil_upload.handle() != m_last_stencil_buffer || target_buffer != m_last_target_buffer) {
        flush();
        m_last_stencil_buffer = stencil_upload.handle();
        m_last_target_buffer  = target_buffer;
    }

    m_copies.push_back(VkBufferCopy{
        .srcOffset = stencil_upload.byte_offset(),
        .dstOffset = offset,
        .size      = data.size(),
    });
}

void BatchUploader::flush() {
    if(m_copies.size() == 0) return;
    vkCmdCopyBuffer(m_cmd->handle(), m_last_stencil_buffer, m_last_target_buffer, m_copies.size(), m_copies.data());
    m_copies.clear();
}

BatchUploader::~BatchUploader() {
    flush();
}

} // namespace vke
