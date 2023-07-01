#pragma once

#include <vector>

#include <cstring>

#include "../core/buffer.hpp"
#include "../fwd.hpp"

namespace vke{

template <typename T>
std::span<const u8> to_bytes(const T& object) {
    return std::span(reinterpret_cast<const u8*>(&object), sizeof(T));
}

template <typename T>
std::span<const u8> to_bytes(std::span<const T> objects) {
    return std::span(reinterpret_cast<const u8*>(objects.data()), objects.size_bytes());
}

class BatchUploader {
public:
    BatchUploader(IStencilBuffer* stencil, vke::CommandBuffer* cmd);
    ~BatchUploader();

    void upload(std::span<const u8>, const IBufferSpan& span);
    void upload(std::span<const u8>, VkBuffer buffer, usize offset);
    void flush();

private:
    std::vector<VkBufferCopy> m_copies;
    vke::CommandBuffer* m_cmd;
    IStencilBuffer* m_stencil;
    VkBuffer m_last_stencil_buffer = nullptr;
    VkBuffer m_last_target_buffer  = nullptr;
};

}

