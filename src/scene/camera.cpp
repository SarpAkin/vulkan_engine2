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
    m_world_position = world_pos;

    update();
}

void Camera::update_view() {
    glm::vec3 local_pos = m_world_position;
    m_view              = glm::lookAt(local_pos, local_pos + forward(), up());
}

void Camera::update() {
    update_view();
    update_proj();

    m_proj_view     = m_proj * m_view;
    m_inv_proj_view = glm::inverse(m_proj_view);
}

void PerspectiveCamera::update_proj() {
    m_proj = z_flip_matrix * glm::perspectiveRH_ZO(fov_deg, aspect_ratio, z_near, z_far);
};

void FreeCamera::move_freecam(Window* window, float delta_time) {
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

    m_yaw_d += window->get_mouse_input().delta_x * sensitivity_x;
    m_pitch_d += window->get_mouse_input().delta_y * sensitivity_y;
    m_pitch_d = std::clamp(m_pitch_d, -89.9f, 89.9f);

    set_rotation_euler(glm::radians(glm::vec3(-m_pitch_d, -m_yaw_d, 0)));

    int z_axis_move = (int)window->is_key_pressed('w') - (int)window->is_key_pressed('s');
    int x_axis_move = (int)window->is_key_pressed('d') - (int)window->is_key_pressed('a');
    int y_axis_move = (int)window->is_key_pressed(' ') - (int)window->is_key_pressed('c');

    glm::vec3 z_axis = forward();
    glm::vec3 x_axis = right();
    glm::vec3 y_axis = up();

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

    m_world_position += total_move;
}

void OrthographicCamera::update_proj() {
    m_proj = z_flip_matrix * glm::orthoRH_ZO(-half_width, half_width, -half_height, half_height, z_near, z_far);
};

} // namespace vke