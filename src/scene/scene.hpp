#pragma once

#include "camera.hpp"

#include <flecs.h>
#include <flecs/addons/flecs_cpp.h>

namespace vke {

class Scene {
public:
    Scene() {};
    ~Scene() {}

    flecs::world* get_world() { return &world; }
    
    Camera* get_camera() { return &m_camera; }

private:
    flecs::world world;

    FreeCamera m_camera;
};

} // namespace vke