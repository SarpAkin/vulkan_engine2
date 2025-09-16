#pragma once

#include <memory>

#include <span>
#include <vulkan/vulkan_core.h>

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vke/fwd.hpp>

#include <vke/util.hpp>
#include <vke/vke.hpp>

namespace vke {

struct AABB {
    glm::vec3 start, end;

    AABB combined(const AABB& other) const {
        return AABB{
            .start = glm::min(start, other.start),
            .end   = glm::max(end, other.end),
        };
    }

    glm::vec3 size() const { return end - start; }
    glm::vec3 half_size() const { return size() / 2.f; }
    glm::vec3 mip_point() const { return start + half_size(); }
};

// this is for caching vkBuffers and their offsets that normally comes from IBufferSpan's virtual getters called at CommandBuffer::bind_vertex_buffers when an array it is passed.
// it avoids costly virtual calls by caching them
class VertexBufferArrayCache {
public:
    VertexBufferArrayCache(auto&& ibuffer_ptrs /*an itertable of IBufferSpan* or std::unique_ptr<IBufferSpan> */) {
        reset(ibuffer_ptrs);
    }

    void reset(auto&& ibuffer_ptrs /*an itertable of IBufferSpan* or std::unique_ptr<IBufferSpan> */) {
        m_handles.clear();
        m_offsets.clear();

        if constexpr (requires { ibuffer_ptrs.size(); }) {
            m_handles.reserve(ibuffer_ptrs.size());
            m_offsets.reserve(ibuffer_ptrs.size());
        }

        for (const auto& ptr : ibuffer_ptrs) {
            m_handles.push_back(ptr->handle());
            m_offsets.push_back(static_cast<VkDeviceSize>(ptr->byte_offset()));
        }
    }

    VertexBufferArrayCache() {}

    VertexBufferArrayCache(const VertexBufferArrayCache&)                = default;
    VertexBufferArrayCache& operator=(const VertexBufferArrayCache&)     = default;
    VertexBufferArrayCache(VertexBufferArrayCache&&) noexcept            = default;
    VertexBufferArrayCache& operator=(VertexBufferArrayCache&&) noexcept = default;

    std::span<const VkBuffer> handles() const { return m_handles; }
    std::span<const VkDeviceSize> offsets() const { return m_offsets; }

private:
    vke::SlimVec<VkBuffer> m_handles;
    vke::SlimVec<VkDeviceSize> m_offsets;
};

struct Mesh {
    struct Vertex {
        glm::vec3 pos;
        u32 color;
    };
    std::unique_ptr<vke::IBufferSpan> index_buffer;
    vke::SlimVec<std::unique_ptr<vke::IBufferSpan>> vertex_buffers;

    VkIndexType index_type = VK_INDEX_TYPE_NONE_KHR;
    uint32_t index_count; // if index_type is VK_INDEX_TYPE_NONE than it means vertex_count
    AABB boundary;

    VertexBufferArrayCache vba_cache;

    void update_vba() { vba_cache.reset(vertex_buffers); }

public:
    Mesh()                                 = default;
    Mesh& operator=(Mesh&& other) noexcept = default;
    Mesh(Mesh&&) noexcept                  = default;
    ~Mesh();
};

class MeshBuilder {
public:
    void set_positions(std::span<glm::vec3> span) { m_positions = span; }
    void set_texture_coords(std::span<glm::vec2> span) { m_texture_coords = span; }
    void set_normals(std::span<glm::vec3> span) { m_normals = span; }
    void set_boundary(const AABB& boundary) { m_boundary = boundary; }

    void set_indicies(std::span<uint16_t> span);
    void set_indicies(std::span<uint32_t> span);

    Mesh build(vke::CommandBuffer* = nullptr, StencilBuffer* stencil = nullptr) const;

    void calculate_boundary();

private:
    std::span<glm::vec3> m_positions       = {};
    std::span<glm::vec3> m_normals         = {};
    std::span<glm::vec2> m_texture_coords  = {};
    std::span<uint8_t> m_indicies_in_bytes = {};
    VkIndexType m_index_type;
    AABB m_boundary = AABB{.start = glm::vec3(NAN), .end = glm::vec3(NAN)};
};

} // namespace vke