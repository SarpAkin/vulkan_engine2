#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "../fwd.hpp"

namespace vke {

class Camera {
public:
    Camera(vke::RenderEngine* engine = nullptr);

    vke::RenderEngine* engine() { return m_engine; }

    virtual void calculate_proj_view() = 0;
    virtual void update(){};

    glm::mat4 inv_proj_view() { return glm::inverse(proj_view); }

public:
    glm::mat4 proj_view;
    glm::vec3 pos = glm::vec3(0, 0, 0);
    glm::vec3 dir = glm::vec3(0, 0, 1.0);
    glm::vec3 up  = glm::vec3(0.0, 1.0, 0.0);

protected:
    vke::RenderEngine* m_engine;
};

class PerspectiveCamera : public Camera {
public:
    void calculate_proj_view() override;

    PerspectiveCamera(vke::RenderEngine* engine = nullptr) : Camera(engine) {}

    float fov          = 90.0; // degrees
    float aspect_ratio = 16.f / 9.f;
    float zfar = 1000.0, znear = 0.1;
};

class FreeMoveCamera : public PerspectiveCamera {
public:
    FreeMoveCamera(vke::RenderEngine* engine);

    // moved pixel * sensivity = rotation in radians
    float sensivity_x = -0.01, sensivity_y = 0.01;
    float speed = 5.0, speed_vertical = 3.0;
    float sprint_mul = 5.5f;

    void update() override;

private:
    float m_pitch = 0.0, m_yaw = 0.0;
};

} // namespace vke
