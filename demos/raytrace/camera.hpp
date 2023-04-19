#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <vke/fwd.hpp>

class Camera {
public:
    Camera(vke::RenderEngine* engine);

    void free_move();
    void calculate_proj_view();

public:
    glm::mat4 proj_view;
    glm::vec3 pos = glm::vec3(0, 0, 0), dir = glm::vec3(0, 0, 1.0), up = glm::vec3(0.0, 1.0, 0.0);
    float fov = 90.0; // degrees
    float aspect_ratio;
    float zfar = 1000.0, znear = 0.1;

    // moved pixel * sensivity = rotation in radians
    float sensivity_x = -0.01, sensivity_y = 0.01;
    float speed = 5.0, speed_vertical = 3.0;

private:
    float m_pitch = 0.0, m_yaw = 0.0;
    vke::RenderEngine* m_engine;
};