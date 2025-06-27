#include "mesh.hpp"

#include <cstring>
#include <vke/util.hpp>
#include <vke/vke.hpp>

namespace vke {

Mesh::~Mesh() {}
void MeshBuilder::set_indicies(std::span<uint16_t> span) {
    m_indicies_in_bytes = vke::span_cast<uint8_t>(span);
    m_index_type        = VK_INDEX_TYPE_UINT16;
}

void MeshBuilder::set_indicies(std::span<uint32_t> span) {
    m_indicies_in_bytes = vke::span_cast<uint8_t>(span);
    m_index_type        = VK_INDEX_TYPE_UINT32;
}

Mesh MeshBuilder::build(vke::CommandBuffer* cmd, StencilBuffer* stencil) const {
    // size_t total_byte_size = 0;
    // total_byte_size += m_positions.size_bytes();
    // total_byte_size += m_texture_coords.size_bytes();
    // total_byte_size += m_normals.size_bytes();
    // total_byte_size += m_indicies_in_bytes.size_bytes();

    Mesh mesh;

    auto create_buffer = [&]<class T>(VkBufferUsageFlags usage, std::span<T> data) {
        bool create_on_device = (cmd != nullptr) && (stencil != nullptr);

        std::unique_ptr<vke::Buffer> buffer = std::make_unique<vke::Buffer>(usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data.size_bytes(), !create_on_device);

        if (create_on_device) {
            stencil->copy_data(buffer->subspan(0), vke::span_cast<u8>(data));
        } else {
            memcpy(buffer->mapped_data_bytes().data(), data.data(), data.size_bytes());
        }
        return buffer;
    };

    auto add_buffer = [&]<class T>(VkBufferUsageFlags usage, std::span<T> data) {
        mesh.vertex_buffers.push_back(create_buffer(usage, data));
    };

    add_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_positions);
    add_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_texture_coords);
    add_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_normals);

    if (m_indicies_in_bytes.size() > 0) {
        mesh.index_buffer = create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_indicies_in_bytes);
        mesh.index_count  = m_indicies_in_bytes.size_bytes() / (m_index_type == VK_INDEX_TYPE_UINT16 ? 2 : 4);
        mesh.index_type   = m_index_type;

    } else {
        mesh.index_type  = VK_INDEX_TYPE_NONE_KHR;
        mesh.index_count = m_positions.size();
    }

    mesh.boundary = m_boundary;

    mesh.update_vba();
    return mesh;
}
} // namespace vke