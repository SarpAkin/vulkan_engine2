#pragma once

#include <flecs.h>

#include <entt/entt.hpp>

namespace vke{

class Transform;

entt::entity instantiate_prefab(entt::registry& scene, const entt::registry& prefab,const std::optional<Transform>& t);
}