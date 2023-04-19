#include "camera.hpp"

#include <vke/engine.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>

Camera::Camera(vke::RenderEngine* engine) {
    m_engine = engine;

    m_engine->window()->on_mouse_down(SDL_BUTTON_LEFT, [engine] {
        engine->window()->lock_mouse();
    });

    m_engine->window()->on_key_down(SDLK_ESCAPE, [engine] {
        engine->window()->unlock_mouse();
    });
}

void Camera::free_move() {
    auto* window = m_engine->window();

    m_yaw += window->get_mouse_input().delta_x * sensivity_x;
    m_pitch += window->get_mouse_input().delta_y * sensivity_y;
    m_pitch = std::clamp(m_pitch, -M_PIf / 2 + 0.001f, M_PIf / 2 - 0.001f);

    float yaw_sin   = std::sin(m_yaw);
    float yaw_cos   = std::cos(m_yaw);
    float pitch_sin = std::sin(m_pitch);
    float pitch_cos = std::cos(m_pitch);

    // z+ forward on yaw 0
    dir = glm::vec3(
        yaw_cos * pitch_cos,
        pitch_sin,
        yaw_sin * pitch_cos //
    );

    float delta_time = m_engine->get_delta_time();

    int z_axis_move = (int)window->is_key_pressed('w') - (int)window->is_key_pressed('s');
    int x_axis_move = (int)window->is_key_pressed('a') - (int)window->is_key_pressed('d');
    int y_axis_move = (int)window->is_key_pressed(' ') - (int)window->is_key_pressed('c');

    glm::vec3 z_axis = dir;
    glm::vec3 x_axis = glm::normalize(glm::cross(up, dir));
    glm::vec3 y_axis = glm::normalize(glm::cross(z_axis, x_axis));

    glm::vec3 total_move = glm::vec3(0, 0, 0);
    total_move += z_axis * float(z_axis_move) * speed;
    total_move += x_axis * float(x_axis_move) * speed;
    total_move += y_axis * float(y_axis_move) * speed_vertical;
    total_move *= delta_time;

    pos += total_move;

    calculate_proj_view();
}

void Camera::calculate_proj_view() {
    if (m_engine) {
        aspect_ratio = (float)m_engine->window()->width() / (float)m_engine->window()->height();
    }

    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect_ratio, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(pos, pos + dir, up);

    proj[1][1] *= -1.0;
    proj_view = proj * view;
}
