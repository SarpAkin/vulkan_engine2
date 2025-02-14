#include "camera.hpp"

#include <algorithm>

#include <vke/vke.hpp>

#include <SDL2/SDL_keycode.h>

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

void Camera::move_freecam(Window* window, float delta_time) {
    static bool focused = false;

    const float sensivity_x    = 0.5;
    const float sensivity_y    = 0.5;
    const float speed          = 35.f;
    const float speed_vertical = 20;
    const float sprint_mul     = 15.f;

    if (window->is_key_pressed('1')) {
        window->lock_mouse();
        focused = true;
    }

    if (window->is_key_pressed(27)) {
        window->unlock_mouse();
        focused = false;
    }

    // printf("mouse delta: (%.2f,%.2f)\n", window->get_mouse_input().delta_x, window->get_mouse_input().delta_y);
    // printf("pitch,yaw: (%.1f,%.1f)\n",pitch,yaw);
    // printf("camera world position: (%.1f,%.1f,%.1f)\n", world_position.x, world_position.y, world_position.z);

    if (focused) {
        yaw += window->get_mouse_input().delta_x * sensivity_x;
        pitch += window->get_mouse_input().delta_y * sensivity_y;
        pitch = std::clamp(pitch, -89.9f, 89.9f);
    }

    float yaw_sin   = std::sin(glm::radians(yaw));
    float yaw_cos   = std::cos(glm::radians(yaw));
    float pitch_sin = std::sin(glm::radians(pitch));
    float pitch_cos = std::cos(glm::radians(pitch));

    // z+ forward on yaw 0
    forward = glm::vec3(
        yaw_cos * pitch_cos,
        pitch_sin,
        yaw_sin * pitch_cos //
    );

    int z_axis_move = (int)window->is_key_pressed('w') - (int)window->is_key_pressed('s');
    int x_axis_move = (int)window->is_key_pressed('d') - (int)window->is_key_pressed('a');
    int y_axis_move = (int)window->is_key_pressed(' ') - (int)window->is_key_pressed('c');

    glm::vec3 z_axis = forward;
    glm::vec3 x_axis = glm::normalize(glm::cross(UP, forward));
    glm::vec3 y_axis = glm::normalize(glm::cross(z_axis, x_axis));

    glm::vec3 total_move = glm::vec3(0, 0, 0);
    total_move += z_axis * float(z_axis_move) * speed;
    total_move += x_axis * float(x_axis_move) * speed;
    total_move += y_axis * float(y_axis_move) * speed_vertical;
    if (window->is_key_pressed(SDLK_LSHIFT)) {
        total_move *= sprint_mul;
    }
    // printf("camera move: (%.1f,%.1f,%.1f)\n", total_move.x, total_move.y, total_move.z);
    // printf("delta time %.2f\n",delta_time);
    total_move *= delta_time;

    world_position += total_move;
}

} // namespace vke