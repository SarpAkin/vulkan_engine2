#pragma once

#include "camera.hpp"
#include <entt/entt.hpp>

namespace vke {

class Scene {
public:
    Scene(){};
    ~Scene(){}

    entt::registry* get_registry() { return &m_registry; }
    Camera* get_camera() { return &m_camera; }

    
private:
    entt::registry m_registry;
    FreeCamera m_camera;
};

} // namespace vke