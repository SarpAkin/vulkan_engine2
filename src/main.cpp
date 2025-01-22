#include <cstdlib>

#include <vke/vke.hpp>

#include "game_engine.hpp"
#include "render/object_renderer.hpp"
#include "render/render_server.hpp"

#include "scene/components/transform.hpp"
#include "scene/scene.hpp"

class Game1 : public vke::GameEngine {
public:
    void populate_scene() {
        auto* registry    = get_scene()->get_registery();
        auto cube_mesh_id = get_render_server()->get_object_renderer()->get_model_id("cube");

        for (int i = 0; i < 10; ++i) {
            auto entity = registry->create();
            registry->emplace<vke::Transform>(entity,
                vke::Transform{
                    .position = glm::dvec3(rand() % 20, rand() % 20, rand() % 20),
                });
            registry->emplace<vke::Renderable>(entity, vke::Renderable{.model_id = cube_mesh_id});
        }
    }

private:
};

int main() {
    Game1 game;
    game.populate_scene();
    game.run();
}