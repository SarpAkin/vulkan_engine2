#include "game_engine.hpp"

#include "imgui.h"
#include "render/object_renderer/object_renderer.hpp"
#include "render/render_pipeline/defered_render_pipeline.hpp"
#include "render/render_server.hpp"
#include "render/render_system.hpp"
#include "scene/scene.hpp"

#include "scene/ui/instantiate_menu.hpp"
#include "ui/imenu.hpp"

namespace vke {

static GameEngine* s_instance = nullptr;

GameEngine::GameEngine(bool headless) {
    m_scene = std::make_unique<Scene>();

    if (!headless) {
        m_renderer = std::make_unique<RenderSystem>(this);
    }

    assert(s_instance == nullptr);
    s_instance = this;

    m_menus = {
        std::make_shared<InstantiateMenu>(this),
    };
}

GameEngine::~GameEngine() {
    assert(this == s_instance);
    s_instance = nullptr;
}

void GameEngine::run() {
    auto prev_time = std::chrono::system_clock::now();

    double time_counter       = 0.0;
    double longest_frame_time = 0.0;
    double fps = 0.0, fps_low = 0.0;

    time_counter = 0.0;

    int fps_frame_counter = 0;
    while (m_running) {
        auto now          = std::chrono::system_clock::now();
        double frame_time = static_cast<std::chrono::duration<double>>((now - prev_time)).count();
        m_run_time += frame_time;
        time_counter += frame_time;
        longest_frame_time = std::max(frame_time, longest_frame_time);

        if (time_counter > 1.0) {
            fps                = static_cast<double>(fps_frame_counter) / time_counter;
            fps_low            = 1 / longest_frame_time;
            time_counter       = 0.0;
            fps_frame_counter  = 0;
            longest_frame_time = 0.0;
        }

        m_delta_time = frame_time;
        prev_time    = now;

        on_update();

        if (m_renderer) {
            if (!m_renderer->get_render_server()->is_running()) break;

            m_renderer->get_render_server()->frame([&](RenderServer::FrameArgs& args) {
                static bool window_opened = true;
                if (window_opened) {
                    ImGui::Begin("Stats", &window_opened);
                    ImGui::Text("FPS: %.1f", fps);
                    ImGui::Text("FPS Low: %.1f", fps_low);
                    ImGui::End();
                }

                for (auto& menu : m_menus) {
                    menu->draw_menu();
                }

                on_render(args);
            });
        }

        fps_frame_counter++;
        m_frame_counter++;
    }
}

void GameEngine::default_render(RenderServer::FrameArgs& args) { m_renderer->render(args); }

RenderServer* GameEngine::get_render_server() { return m_renderer->get_render_server(); }

GameEngine* GameEngine::get_instance() { return s_instance; }
} // namespace vke