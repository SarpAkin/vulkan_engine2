#pragma once

#include "mesh.hpp"

#include <memory>

namespace vke{

class VertexInputDescriptionBuilder;

std::unique_ptr<vke::VertexInputDescriptionBuilder> make_default_vertex_layout();
}