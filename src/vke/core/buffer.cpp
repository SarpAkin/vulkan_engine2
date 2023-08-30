#include "buffer.hpp"

#include "core.hpp"
#include "vk_resource.hpp"
#include "vkutil.hpp"

#include <atomic>
#include <cstring>
#include <memory>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace vke {

Buffer::Buffer(Core* _core, VkBufferUsageFlags usage, usize buffer_size, bool host_visible)
    : Resource(_core) {
    m_buffer_byte_size = buffer_size;

    VkBufferCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = buffer_size,
        .usage = usage,
    };

    VmaAllocationCreateInfo alloc_info{
        .flags = host_visible ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0u,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VK_CHECK(vmaCreateBuffer(core()->gpu_allocator(), &create_info, &alloc_info, &m_buffer, &m_allocation, nullptr));
    vmaSetAllocationName(core()->gpu_allocator(), m_allocation, "image");

    if (host_visible) {
        VK_CHECK(vmaMapMemory(core()->gpu_allocator(), m_allocation, &m_mapped_data));
    }

#ifndef NDEBUG
    core()->buffer_counter++;
#endif
}

std::unique_ptr<Buffer> Core::create_buffer(VkBufferUsageFlags usage, usize buffer_size, bool host_visible /* whether it is accessible by cpu*/) {
    return std::make_unique<Buffer>(this, usage, buffer_size, host_visible);
}

// creates host visible data initalized with given bytes
std::unique_ptr<Buffer> Core::create_buffer_from_data(VkBufferUsageFlags usage, std::span<u8> bytes) {
    auto buffer = create_buffer(usage, bytes.size_bytes(), true);
    memcpy(buffer->mapped_data_ptr(), bytes.data(), bytes.size_bytes());
    return buffer;
}

Buffer::~Buffer() {
#ifndef NDEBUG
    core()->buffer_counter--;
#endif

    if (m_mapped_data) {
        vmaUnmapMemory(core()->gpu_allocator(), m_allocation);
    }

    vmaDestroyBuffer(core()->gpu_allocator(), m_buffer, m_allocation);
}

BufferSpan IBufferSpan::subspan(usize _byte_offset, usize _byte_size) {
    return BufferSpan(vke_buffer(), byte_offset() + _byte_offset, std::min(_byte_size, byte_size() - _byte_offset));
}

} // namespace vke