#include "mesh.hpp"

#include <cstring>
#include <vke/util.hpp>
#include <vke/vke.hpp>

namespace vke {

std::unique_ptr<Mesh> Mesh::make_mesh(std::span<Vertex> verticies, std::span<uint16_t> indicies) {

    auto vb = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, verticies.size_bytes(), true);
    memcpy(vb->mapped_data_bytes().data(), verticies.data(), verticies.size_bytes());

    auto ib = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indicies.size_bytes(), true);
    memcpy(ib->mapped_data_bytes().data(), indicies.data(), indicies.size_bytes());

    auto mesh = std::make_unique<Mesh>();
    ;
    mesh->vertex_buffer = std::move(vb);
    mesh->index_buffer  = std::move(ib);
    mesh->index_count   = indicies.size();
    mesh->index_type    = VK_INDEX_TYPE_UINT16;

    return mesh;
}

Mesh::~Mesh() {}
void MeshBuilder::set_indicies(std::span<uint16_t> span) {
    m_indicies_in_bytes = vke::span_cast<uint8_t>(span);
    m_index_type        = VK_INDEX_TYPE_UINT16;
}

void MeshBuilder::set_indicies(std::span<uint32_t> span) {
    m_indicies_in_bytes = vke::span_cast<uint8_t>(span);
    m_index_type        = VK_INDEX_TYPE_UINT32;
}

Mesh MeshBuilder::build(vke::CommandBuffer* buffer) const {
    // size_t total_byte_size = 0;
    // total_byte_size += m_positions.size_bytes();
    // total_byte_size += m_texture_coords.size_bytes();
    // total_byte_size += m_normals.size_bytes();
    // total_byte_size += m_indicies_in_bytes.size_bytes();

    Mesh mesh;

    auto vb = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_positions.size_bytes(), true);
    memcpy(vb->mapped_data_bytes().data(), m_positions.data(), m_positions.size_bytes());
    mesh.vertex_buffer = std::move(vb);

    if (m_indicies_in_bytes.size() > 0) {
        auto ib = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_indicies_in_bytes.size_bytes(), true);
        memcpy(ib->mapped_data_bytes().data(), m_indicies_in_bytes.data(), m_indicies_in_bytes.size_bytes());
        mesh.index_buffer = std::move(ib);
        mesh.index_count  = m_indicies_in_bytes.size_bytes() / (m_index_type == VK_INDEX_TYPE_UINT16 ? 2 : 4);
        mesh.index_type   = m_index_type;

    } else {
        mesh.index_type  = VK_INDEX_TYPE_NONE_KHR;
        mesh.index_count = m_positions.size();
    }

    return mesh;
}
} // namespace vke