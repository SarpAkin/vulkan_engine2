#pragma once

#include <entt/fwd.hpp>

#include "render/object_renderer/object_renderer.hpp"

namespace vke{

void load_gltf_file(vke::CommandBuffer& cmd,entt::registry* registry,ObjectRenderer* renderer, const std::string& file_path);
}