#include "game_engine.hpp"

#include "imgui.h"
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
        m_render_server->get_object_renderer()->set_scene(m_scene.get());
    }
}

void GameEngine::run() {
    auto prev_time = std::chrono::system_clock::now();

    double time_counter       = 0.0;
    double total_time_counter = 0.0;
    double longest_frame_time = 0.0;

    double fps = 0.0, fps_low = 0.0;

    int frame_counter = 0;
    while (m_running) {
        auto now          = std::chrono::system_clock::now();
        double frame_time = static_cast<std::chrono::duration<double>>((now - prev_time)).count();
        total_time_counter += frame_time;
        time_counter += frame_time;
        longest_frame_time = std::max(frame_time, longest_frame_time);

        if (time_counter > 1.0) {
            fps                = static_cast<double>(frame_counter) / time_counter;
            fps_low            = 1 / longest_frame_time;
            time_counter       = 0.0;
            frame_counter      = 0;
            longest_frame_time = 0.0;
        }

        m_delta_time = frame_time;
        prev_time    = now;

        on_update();

        if (m_render_server) {
            if (!m_render_server->is_running()) break;

            m_render_server->frame([&](vke::CommandBuffer& cmd) {
                static bool window_opened = false;
                if (window_opened) {
                    ImGui::Begin("Stats", &window_opened);
                    ImGui::Text("FPS: %.1f", fps);
                    ImGui::Text("FPS Low: %.1f", fps_low);
                    ImGui::End();
                }

                on_render(cmd);
            });
        }

        frame_counter++;
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