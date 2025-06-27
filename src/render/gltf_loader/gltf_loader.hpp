#pragma once


#include <flecs.h>
#include <flecs/addons/flecs_cpp.h>

#include <entt/fwd.hpp>

#include "fwd.hpp"
#include <optional>
#include <vke/fwd.hpp>


namespace vke {

void load_gltf_file(vke::CommandBuffer& cmd, entt::registry* registry, ObjectRenderer* renderer, const std::string& file_path);

std::optional<flecs::entity> load_gltf_file(vke::CommandBuffer& cmd, flecs::world*, ObjectRenderer* renderer, const std::string& file_path);

} // namespace vke