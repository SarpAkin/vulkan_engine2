#include <cstdlib>

#include <imgui.h>

#include <vke/vke.hpp>

#include "game_engine.hpp"
#include "render/gltf_loader/gltf_loader.hpp"
#include "render/object_renderer/object_renderer.hpp"
#include "render/render_server.hpp"

#include "scene/components/transform.hpp"
#include "scene/components/util.hpp"
#include "scene/scene.hpp"

glm::quat random_quaternion() {
    // Generate a random axis (unit vector) and angle
    float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * glm::pi<float>() * 2.0f; // Angle between 0 and 2π

    // Generate a random unit vector for axis of rotation
    glm::vec3 axis = glm::normalize(glm::vec3(
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f, // Random X between -1 and 1
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f, // Random Y between -1 and 1
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f  // Random Z between -1 and 1
        ));

    // Create a quaternion representing a rotation by the random angle around the random axis
    glm::quat q = glm::angleAxis(angle, axis);
    return q;
}

class Game1 : public vke::GameEngine {
public:
    void populate_scene() {
        auto* registry = get_scene()->get_registry();
        // auto cube_mesh_id = get_render_server()->get_object_renderer()->get_model_id("cube");

        vke::VulkanContext::get_context()->immediate_submit([&](vke::CommandBuffer& cmd) {
            vke::load_gltf_file(cmd, &m_prefabs["city"], get_render_server()->get_object_renderer(), ".misc/gltf_models/scene.gltf");
            vke::load_gltf_file(cmd, &m_prefabs["cube"], get_render_server()->get_object_renderer(), ".misc/gltf_models/block.glb");
        });

        vke::instantiate_prefab(*registry, m_prefabs["cube"],
            vke::Transform{
                .position = {0, 0, 0},
                .scale    = {1, 1, 1},
            });

        for (int i = 0; i < 30; ++i) {
            vke::instantiate_prefab(*registry, m_prefabs["cube"],
                vke::Transform{
                    .position = glm::dvec3(rand() % 20, rand() % 20, rand() % 20),
                    .rotation = random_quaternion(),
                    .scale    = glm::vec3(1),
                });
        }
    }

    void on_update() override {
    }

    void on_render(vke::RenderServer::FrameArgs& args) override {
        default_render(args);

        instantiate_menu();
    }

    void instantiate_menu() {
        auto* registry = get_scene()->get_registry();
        auto* window   = get_render_server()->get_window();
        auto* player   = get_scene()->get_camera();

        ImGui::Begin("instantiate");

        if (ImGui::CollapsingHeader("Light")) {
            static float range = 5.f, strength = 2.f;
            const float min_range = 0.1f, max_range = 100.f;
            ImGui::SliderFloat("range", &range, min_range, max_range);
            ImGui::SliderFloat("strength", &strength, 0.1f, 20.f);

            static float color[4] = {};
            if (ImGui::CollapsingHeader("color")) {
                ImGui::ColorPicker3("color", color);
            }

            if (ImGui::Button("add light")) {
                auto entity = registry->create();
                registry->emplace<vke::Transform>(entity, vke::Transform{.position = player->world_position});
                registry->emplace<vke::CPointLight>(entity, vke::CPointLight{.color = glm::vec3(color[0], color[1], color[2]) * strength, .range = range});
            }
        }

        if (ImGui::CollapsingHeader("prefab")) {
            static std::string selectedPrefab;

            if (ImGui::BeginListBox("Prefab Selection")) {
                for (const auto& [prefab_name, prefab] : m_prefabs) {
                    bool isSelected = (prefab_name == selectedPrefab);
                    if (ImGui::Selectable(prefab_name.c_str(), isSelected)) {
                        selectedPrefab = prefab_name;
                    }
                }
                ImGui::EndListBox();
            }

            static char buffer[64] = "r(0,0,0)";
            ImGui::InputText("coordinate", buffer, vke::array_len(buffer));

            double x = 0, y = 0, z = 0, sx = 1.0, sy = 1.0, sz = 1.0;
            char c = '\0';
            if (int count = sscanf(buffer, "%c(%lf,%lf,%lf) (%lf,%lf,%lf)", &c, &x, &y, &z, &sx, &sy, &sz); count >= 4) {
                vke::Transform t{
                    .position = {x, y, z},
                    .rotation = {},
                    .scale    = glm::vec3(1),
                };

                if (c == 'r') {
                    t.position += player->world_position;
                }

                if (count >= 7) {
                    t.scale = {sx, sy, sz};
                }

                if (ImGui::Button("spawn")) {
                    vke::instantiate_prefab(*registry, m_prefabs[selectedPrefab], t);
                }

            } else {
                ImGui::Text("invalid coordinate");
            }
        }

        ImGui::End();
    }

private:
    std::unordered_map<std::string, entt::registry> m_prefabs;
};

int main() {
    Game1 game;
    game.populate_scene();
    game.run();
}
