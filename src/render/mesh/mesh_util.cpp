#include "mesh_util.hpp"

#include <vke/vke_builders.hpp>

namespace vke{

std::unique_ptr<vke::VertexInputDescriptionBuilder> make_default_vertex_layout() {
    auto builder = std::make_unique<vke::VertexInputDescriptionBuilder>();

    builder->push_binding<glm::vec3>();
    builder->push_attribute<float>(3);

    builder->push_binding<glm::vec2>();
    builder->push_attribute<float>(2);

    // builder->push_attribute<u32>(1);

    return builder;
}
} // namespace vke