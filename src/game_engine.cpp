#include "game_engine.hpp"

#include "render/object_renderer.hpp"
#include "render/render_server.hpp"
#include "scene/scene.hpp"

namespace vke {

GameEngine::GameEngine(bool headless) {
    m_scene = std::make_unique<Scene>();

    if (!headless) {
        m_render_server = std::make_unique<RenderServer>();
        m_render_server->init();

        m_render_server->get_object_renderer()->set_entt_registery(m_scene->get_registery());
        m_render_server->get_object_renderer()->set_camera(m_scene->get_camera());
    }
}

void GameEngine::run() {
    auto prev_time = std::chrono::system_clock::now();

    while (m_running) {
        auto now     = std::chrono::system_clock::now();
        m_delta_time = static_cast<std::chrono::duration<double>>((now - prev_time)).count();
        prev_time    = now;

        on_update();

        if (m_render_server) {
            if (!m_render_server->is_running()) break;

            m_render_server->frame([&](vke::CommandBuffer& cmd) {
                on_render(cmd);
            });
        }
    }
}

void GameEngine::default_render(vke::CommandBuffer& cmd) {
    auto* cam    = get_scene()->get_camera();
    auto* window = get_render_server()->get_window();

    cam->aspect_ratio = static_cast<float>(window->width()) / static_cast<float>(window->height());
    cam->move_freecam(window, get_delta_time());
    cam->update();

    m_render_server->get_object_renderer()->render(cmd);
}
} // namespace vke