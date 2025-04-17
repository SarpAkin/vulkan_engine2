#include "camera.hpp"

#include <algorithm>

#include <vke/vke.hpp>

#include <SDL2/SDL_keycode.h>

#include <imgui.h>

namespace vke {

constexpr glm::mat4 z_flip_matrix = glm::mat4(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, -1, 0,
    0, 0, 1, 1 //
);

void Camera::set_world_pos(const glm::dvec3& world_pos) {
    world_position = world_pos;

    update();
}

void PerspectiveCamera::update() {
    // m_proj = perspective_custom(fov_deg, aspect_ratio, z_near, z_far);

    m_proj = z_flip_matrix * glm::perspectiveRH_ZO(fov_deg, aspect_ratio, z_near, z_far);

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

    static float sensitivity_x  = 0.5;
    static float sensitivity_y  = 0.5;
    static float speed          = 35.f;
    static float speed_vertical = 20;
    static float sprint_mul     = 15.f;

    ImGui::Begin("FreeCam");
    ImGui::SliderFloat("speed", &speed, 1.f, 100.f);
    ImGui::SliderFloat("vertical speed", &speed_vertical, 1.f, 100.f);
    ImGui::SliderFloat("speed multiplier", &sprint_mul, 1.f, 100.f);
    ImGui::End();

    if (window->is_key_pressed('"')) {
        window->lock_mouse();
        focused = true;
    }

    if (window->is_key_pressed(27)) {
        window->unlock_mouse();
        focused = false;
    }

    if (!focused) return;

    // printf("mouse delta: (%.2f,%.2f)\n", window->get_mouse_input().delta_x, window->get_mouse_input().delta_y);
    // printf("pitch,yaw: (%.1f,%.1f)\n",pitch,yaw);
    // printf("camera world position: (%.1f,%.1f,%.1f)\n", world_position.x, world_position.y, world_position.z);

    yaw += window->get_mouse_input().delta_x * sensitivity_x;
    pitch += window->get_mouse_input().delta_y * sensitivity_y;
    pitch = std::clamp(pitch, -89.9f, 89.9f);

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