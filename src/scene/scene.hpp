#pragma once

#include "camera.hpp"
#include <entt/entt.hpp>

namespace vke {

class Scene {
public:
    Scene(){};
    ~Scene(){}

    entt::registry* get_registery() { return &m_registery; }
    Camera* get_camera() { return &m_camera; }

    
private:
    entt::registry m_registery;
    Camera m_camera;
};

} // namespace vke