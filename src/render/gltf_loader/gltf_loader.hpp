#pragma once

#include <entt/fwd.hpp>

#include "../object_renderer.hpp"

namespace vke{

void load_gltf_file(vke::CommandBuffer& cmd,entt::registry* registery,ObjectRenderer* renderer, const std::string& file_path);
}