#include "mesh.hpp"

#include <cstring>
#include <vke/vke.hpp>

namespace vke{

std::unique_ptr<Mesh> Mesh::make_mesh(std::span<Vertex> verticies,std::span<uint16_t> indicies) {
    
    auto vb = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,verticies.size_bytes(),true);
    memcpy(vb->mapped_data_bytes().data(), verticies.data(), verticies.size_bytes());

    auto ib = std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,indicies.size_bytes(),true);
    memcpy(ib->mapped_data_bytes().data(), indicies.data(), indicies.size_bytes());

    auto mesh = std::make_unique<Mesh>();;
    mesh->vertex_buffer = std::move(vb);
    mesh->index_buffer = std::move(ib);
    mesh->index_count = indicies.size();
    mesh->index_type = VK_INDEX_TYPE_UINT16;

    return mesh;
}

Mesh::~Mesh() {}
} // namespace vke