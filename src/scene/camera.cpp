#include "camera.hpp"

#include <algorithm>

#include <vke/vke.hpp>

namespace vke {

void Camera::set_world_pos(const glm::dvec3& wpos) {
    world_position = wpos;

    update();
}

void Camera::update() {
    m_proj = glm::perspective(fov_deg, aspect_ratio, z_near, z_far);

    
    float yaw_sin   = std::sin(glm::radians(yaw));
    float yaw_cos   = std::cos(glm::radians(yaw));
    float pitch_sin = std::sin(glm::radians(pitch));
    float pitch_cos = std::cos(glm::radians(pitch));

    pitch_cos = std::max(0.001f, pitch_cos);

    // z+ forward on yaw 0
    forward = glm::vec3(
        yaw_cos * pitch_cos,
        pitch_sin,
        yaw_sin * pitch_cos //
    );

    glm::vec3 local_pos = static_cast<glm::vec3>(world_position);

    m_view = glm::lookAt(local_pos, local_pos + forward, UP);

    m_proj_view = m_proj * m_view;
}



void Camera::move_freecam(Window* window,float delta_time) {
    const float sensivity_x = 0.05;
    const float sensivity_y = 0.05;
    const float speed = 5.0;
    const float speed_vertical = 2;

    yaw += window->get_mouse_input().delta_x * sensivity_x;
    pitch += window->get_mouse_input().delta_y * sensivity_y;
    pitch = std::clamp(pitch, -M_PIf / 2 + 0.001f, M_PIf / 2 - 0.001f);

    float yaw_sin   = std::sin(yaw);
    float yaw_cos   = std::cos(yaw);
    float pitch_sin = std::sin(pitch);
    float pitch_cos = std::cos(pitch);

    // z+ forward on yaw 0
    forward = glm::vec3(
        yaw_cos * pitch_cos,
        pitch_sin,
        yaw_sin * pitch_cos //
    );

    int z_axis_move = (int)window->is_key_pressed('w') - (int)window->is_key_pressed('s');
    int x_axis_move = (int)window->is_key_pressed('a') - (int)window->is_key_pressed('d');
    int y_axis_move = (int)window->is_key_pressed(' ') - (int)window->is_key_pressed('c');

    glm::vec3 z_axis = forward;
    glm::vec3 x_axis = glm::normalize(glm::cross(UP, forward));
    glm::vec3 y_axis = glm::normalize(glm::cross(z_axis, x_axis));

    glm::vec3 total_move = glm::vec3(0, 0, 0);
    total_move += z_axis * float(z_axis_move) * speed;
    total_move += x_axis * float(x_axis_move) * speed;
    total_move += y_axis * float(y_axis_move) * speed_vertical;
    total_move *= delta_time;

    world_position += total_move;
}


} // namespace vke