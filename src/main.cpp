#include <cstdlib>

#include <vke/vke.hpp>

#include "game_engine.hpp"
#include "render/gltf_loader/gltf_loader.hpp"
#include "render/object_renderer.hpp"
#include "render/render_server.hpp"

#include "scene/components/transform.hpp"
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
        auto* registry    = get_scene()->get_registery();
        // auto cube_mesh_id = get_render_server()->get_object_renderer()->get_model_id("cube");

        // for (int i = 0; i < 30; ++i) {
        //     auto entity = registry->create();
        //     registry->emplace<vke::Transform>(entity,
        //         vke::Transform{
        //             .position = glm::dvec3(rand() % 20, rand() % 20, rand() % 20),
        //             .rotation = random_quaternion(),
        //             .scale = glm::vec3(1),
        //         });
        //     registry->emplace<vke::Renderable>(entity, vke::Renderable{.model_id = cube_mesh_id});
        // }

        vke::VulkanContext::get_context()->immediate_submit([&](vke::CommandBuffer& cmd) {
            vke::load_gltf_file(cmd, registry, get_render_server()->get_object_renderer(), ".misc/gltf_models/scene.gltf");
        });
    }

private:
};

int main() {
    Game1 game;
    game.populate_scene();
    game.run();
}
