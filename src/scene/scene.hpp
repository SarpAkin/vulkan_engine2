#pragma once

#include "camera.hpp"
#include <entt/entt.hpp>

#include <flecs.h>
#include <flecs/addons/flecs_cpp.h>

namespace vke {

class Scene {
public:
    Scene() {};
    ~Scene() {}

    entt::registry* get_registry() { return &m_registry; }
    flecs::world* get_world() { return &world; }
    
    Camera* get_camera() { return &m_camera; }

private:
    entt::registry m_registry;
    flecs::world world;

    FreeCamera m_camera;
};

} // namespace vke