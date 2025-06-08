#include "instantiate_menu.hpp"

#include "game_engine.hpp"
#include "scene/components/transform.hpp"
#include "scene/components/util.hpp"
#include "scene/scene.hpp"
#include <imgui.h>


namespace vke{

void InstantiateMenu::draw_menu() {
    auto* registry = m_game_engine->get_scene()->get_registry();
    auto* window   = m_game_engine->get_render_server()->get_window();
    auto* player   = m_game_engine->get_scene()->get_camera();

    ImGui::Begin("instantiate");

    auto player_pos = player->get_world_pos();
    ImGui::Text("World Position (%lf,%lf,%lf)\n", player_pos.x, player_pos.y, player_pos.z);

    if (ImGui::CollapsingHeader("Light")) {
        const float min_range = 0.1f, max_range = 100.f;
        ImGui::SliderFloat("range", &m_light_range, min_range, max_range);
        ImGui::SliderFloat("strength", &m_light_strength, 0.1f, 20.f);

        if (ImGui::CollapsingHeader("color")) {
            ImGui::ColorPicker3("color", m_picker_color);
        }

        if (ImGui::Button("add light")) {
            auto entity = registry->create();
            registry->emplace<vke::Transform>(entity, vke::Transform{.position = player->get_world_pos()});
            registry->emplace<vke::CPointLight>(entity, vke::CPointLight{.color = glm::vec3(m_picker_color[0], m_picker_color[1], m_picker_color[2]) * m_light_strength, .range = m_light_range});
        }
    }

    if (ImGui::CollapsingHeader("prefab")) {

        if (ImGui::BeginListBox("Prefab Selection")) {
            for (const auto& [prefab_name, prefab] : m_game_engine->get_prefabs()) {
                bool isSelected = (prefab_name == m_selected_prefab);
                if (ImGui::Selectable(prefab_name.c_str(), isSelected)) {
                    m_selected_prefab = prefab_name;
                }
            }
            ImGui::EndListBox();
        }

        ImGui::InputText("coordinate", m_coord_input_buffer, vke::array_len(m_coord_input_buffer));

        double x = 0, y = 0, z = 0, sx = 1.0, sy = 1.0, sz = 1.0, rx = 0.0, ry = 0.0, rz = 0.0;
        char c    = '\0';
        int count = sscanf(m_coord_input_buffer, "%c(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf)", &c, &x, &y, &z, &sx, &sy, &sz, &rx, &ry, &rz);
        if (count >= 4) {
            vke::Transform t{
                .position = {x, y, z},
                .rotation = {},
                .scale    = glm::vec3(1),
            };

            if (c == 'r') {
                t.position += player->get_world_pos();
            }

            if (count >= 7) {
                t.scale = {sx, sy, sz};
            }

            if (count >= 10) {
                t.rotation = glm::quat(glm::radians(glm::vec3(rx, ry, rz)));
            }

            if (ImGui::Button("spawn")) {
                vke::instantiate_prefab(*registry, m_game_engine->get_prefabs()[m_selected_prefab], t);
            }

        } else {
            ImGui::Text("invalid coordinate");
        }
    }

    ImGui::End();
}
} // namespace vke