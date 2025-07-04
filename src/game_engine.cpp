#include "game_engine.hpp"

#include "imgui.h"
#include "render/gltf_loader/gltf_loader.hpp"
#include "render/object_renderer/object_renderer.hpp"
#include "render/render_pipeline/defered_render_pipeline.hpp"
#include "render/render_server.hpp"
#include "render/render_system.hpp"
#include "scene/components/transform.hpp"
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

void GameEngine::load_gltf(vke::CommandBuffer& cmd, const std::string& file_path, const std::string& prefab_name) {
    auto entity = vke::load_gltf_file(cmd, m_scene->get_world(), m_renderer->get_render_server()->get_object_renderer(), file_path);
    if (!entity) {
        LOG_ERROR("failed to load prefab %s. gltf file path : %s", prefab_name.c_str(), file_path.c_str());
        return;
    }
    entity->set_name(prefab_name.c_str());
    // m_prefabs.emplace(std::make_pair(prefab_name, entity.value()));
    m_prefabs[prefab_name] = entity.value();
}

flecs::entity GameEngine::instantiate_prefab(const std::string& prefab_name, std::optional<Transform> transform) {
    auto prefab = m_prefabs.at(prefab_name);
    auto world  = m_scene->get_world();

    auto entity = world->entity().is_a(prefab);

    auto* rel_transform = entity.get<RelativeTransform>();

    auto t1 = transform.value_or(Transform::IDENTITY);

    if (rel_transform) {
        entity.set<Transform>(t1 * static_cast<Transform>(*rel_transform));
        entity.remove<RelativeTransform>();
    } else {
        entity.set<Transform>(t1);
    }

    return entity;
}

flecs::world* GameEngine::get_world() {
    return m_scene->get_world();
}

} // namespace vke