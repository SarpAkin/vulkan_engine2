#include "camera.hpp"

#include "engine.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>

namespace vke {

Camera::Camera(vke::RenderEngine* engine) {
    m_engine = engine;
}

void PerspectiveCamera::calculate_proj_view() {
    if (m_engine) {
        aspect_ratio = (float)m_engine->window()->width() / (float)m_engine->window()->height();
    }

    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect_ratio, znear, zfar);
    glm::mat4 view = glm::lookAt(pos, pos + dir, up);

    if (flip_y) proj[1][1] *= -1.0;
    if (flip_x) proj[0][0] *= -1.0;

    proj_view = proj * view;
}

bool mouse_on_ui = false;

void FreeMoveCamera::update() {
    if (mouse_on_ui) return;
    auto* window = engine()->window();

    aspect_ratio = (float)window->width() / (float)window->height();

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

    float delta_time = engine()->get_delta_time();

    float z_axis_move = (int)window->is_key_pressed('w') - (int)window->is_key_pressed('s');
    float x_axis_move = (int)window->is_key_pressed('a') - (int)window->is_key_pressed('d');
    float y_axis_move = (int)window->is_key_pressed(' ') - (int)window->is_key_pressed('c');

    if (window->is_key_pressed(SDLK_LSHIFT)) {
        z_axis_move *= sprint_mul;
    }

    glm::vec3 z_axis = dir;
    glm::vec3 x_axis = glm::normalize(glm::cross(up, dir));
    glm::vec3 y_axis = glm::normalize(glm::cross(z_axis, x_axis));

    glm::vec3 total_move = glm::vec3(0, 0, 0);
    total_move += z_axis * z_axis_move * speed;
    total_move += x_axis * x_axis_move * speed;
    total_move += y_axis * y_axis_move * speed_vertical;
    total_move *= delta_time;

    pos += total_move;

    calculate_proj_view();
}

FreeMoveCamera::FreeMoveCamera(vke::RenderEngine* _engine, bool set_keybinds) : PerspectiveCamera(_engine) {
    if (!set_keybinds) return;

    engine()->window()->on_key_down(SDLK_u, [_engine] {
        if (!mouse_on_ui) {
            _engine->window()->unlock_mouse();
        }

        mouse_on_ui = !mouse_on_ui;
    });

    engine()->window()->on_mouse_down(SDL_BUTTON_LEFT, [_engine] {
        if (mouse_on_ui) return;
        _engine->window()->lock_mouse();
    });

    engine()->window()->on_key_down(SDLK_ESCAPE, [_engine] {
        _engine->window()->unlock_mouse();
    });
}

} // namespace vke
