#include <cstdlib>

#include <imgui.h>

#include <iostream>
#include <vke/vke.hpp>

#include "game_engine.hpp"
#include "render/gltf_loader/gltf_loader.hpp"
#include "render/object_renderer/object_renderer.hpp"
#include "render/render_server.hpp"

#include "scene/components/transform.hpp"
#include "scene/components/util.hpp"
#include "scene/scene.hpp"

#include <flecs/addons/flecs_cpp.h>

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
            // vke::load_gltf_file(cmd, &get_prefabs()["city"], get_render_server()->get_object_renderer(), ".misc/gltf_models/scene.gltf");
            // vke::load_gltf_file(cmd, &get_prefabs()["cube"], get_render_server()->get_object_renderer(), ".misc/gltf_models/block.glb");
            load_gltf(cmd, ".misc/gltf_models/scene.gltf", "city");
            load_gltf(cmd, ".misc/gltf_models/block.glb", "cube");
        });

        // vke::instantiate_prefab(*registry, m_prefabs["cube"],
        //     vke::Transform{
        //         .position = {0, 0, 0},
        //         .scale    = {1, 1, 1},
        //     });

        // for (int i = 0; i < 5'000; ++i) {
        //     vke::instantiate_prefab(*registry, get_prefabs()["cube"],
        //         vke::Transform{
        //             .position = glm::dvec3((rand() % 2000) - 1000, (rand() % 2000) - 1000, (rand() % 2000) - 1000),
        //             .rotation = random_quaternion(),
        //             .scale    = glm::vec3(1),
        //         });
        // }

        // vke::instantiate_prefab(*registry, get_prefabs()["city"],
        //     vke::Transform{
        //         .position = {0, 0, 0},
        //         .scale    = {1, 1, 1},
        //     });
    }

    void on_update() override {
    }

    void on_render(vke::RenderServer::FrameArgs& args) override {
        default_render(args);

        instantiate_menu();
    }

    void instantiate_menu() {
    }

private:
};

void flecs_print(flecs::entity e, int depth = 0) {
    printf("(%s:%ld)\n", e.name().c_str(), e.id());

    std::string space;
    space.resize(depth * 2, ' ');

    e.children([&](flecs::entity e) {
        printf("%s|-", space.c_str());
        flecs_print(e, depth + 1);
    });
}

void flecs_tests() {
    flecs::world world;

    world.component<vke::Renderable>();
    // world.component<vke::Transform>().add(flecs::OnInstantiate,flecs::Inherit);
    // world.component<vke::RelativeTransform>().add(flecs::OnInstantiate,flecs::Inherit);

    auto dog = world.prefab("dog").set<vke::Renderable>(vke::Renderable{
        .model_id = 11,
    });

    world.prefab().child_of(dog).set(vke::RelativeTransform{
        .position = {-1, 0, 0},
    });

    for (int i = 0; i < 10; i++) {
        char buf[20];
        snprintf(buf, sizeof(buf), "dog %d", i);

        world.entity(buf).is_a(dog).set(vke::Transform{
            {0, 0, 1},
        });
    }

    for (int i = 0; i < 10; i++) {
        world.entity().set<vke::RelativeTransform>(vke::RelativeTransform{
            {i, 0, 0},
        });
    }

    world.each<const vke::RelativeTransform>([&](flecs::entity e, const vke::RelativeTransform& t) {
        printf("%p\n", &t);
    });

    world.query<vke::Transform, vke::Renderable>().each([&](flecs::entity e, vke::Transform& t, vke::Renderable& r) {
        printf("%p : %p\n", &t, &r);
    });
}

int main() {
    Game1 game;
    game.populate_scene();
    game.run();
}
