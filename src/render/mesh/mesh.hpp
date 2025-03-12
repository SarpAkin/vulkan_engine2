#pragma once

#include <memory>

#include <span>
#include <vulkan/vulkan_core.h>

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vke/fwd.hpp>

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
};

struct Mesh {
    struct Vertex {
        glm::vec3 pos;
        u32 color;
    };
    std::unique_ptr<vke::IBufferSpan> index_buffer;
    std::vector<std::unique_ptr<vke::IBufferSpan>> vertex_buffers;
    VkIndexType index_type = VK_INDEX_TYPE_NONE_KHR;
    uint32_t index_count; // if index_type is VK_INDEX_TYPE_NONE than it means vertex_count
    AABB boundary;

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

    Mesh build(vke::CommandBuffer* = nullptr) const;

private:
    std::span<glm::vec3> m_positions       = {};
    std::span<glm::vec3> m_normals         = {};
    std::span<glm::vec2> m_texture_coords  = {};
    std::span<uint8_t> m_indicies_in_bytes = {};
    VkIndexType m_index_type;
    AABB m_boundary = {};
};

} // namespace vke