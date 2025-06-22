#include "render_system.hpp"
#include "render/object_renderer/object_renderer.hpp"
#include "render/render_pipeline/defered_render_pipeline.hpp"
#include "render/object_renderer/light_buffers_manager.hpp"
#include "render/shadow/direct_shadow_map.hpp"
#include "render/shadow/shadow_manager.hpp"
#include "scene/scene.hpp"
#include "game_engine.hpp"


namespace vke{

RenderSystem::RenderSystem(GameEngine* game_engine) {
    m_game_engine = game_engine;
    m_scene = game_engine->get_scene();

    m_render_server = std::make_unique<RenderServer>();
    m_render_server->init();

    const std::string render_target_name = "default";

    auto obj_renderer = m_render_server->get_object_renderer();

    obj_renderer->set_entt_registry(m_scene->get_registry());

    obj_renderer->create_render_target(render_target_name, "vke::default_forward", {true});
    obj_renderer->set_camera(render_target_name, m_scene->get_camera());

    m_render_pipeline = std::make_unique<DeferredRenderPipeline>(m_render_server.get());
    m_render_pipeline->set_camera(m_scene->get_camera());
}

RenderSystem::~RenderSystem() {
    m_render_server->early_cleanup();
}

void RenderSystem::render(RenderServer::FrameArgs& args) {
    auto* cam    = dynamic_cast<FreeCamera*>(m_scene->get_camera());
    auto* window = get_render_server()->get_window();

    cam->aspect_ratio = static_cast<float>(window->width()) / static_cast<float>(window->height());
    cam->move_freecam(window, m_game_engine->get_delta_time());
    cam->update();

    auto* shadow_manager = m_render_server->get_object_renderer()->get_light_manager()->get_shadow_manager();
    
    m_render_server->get_object_renderer()->update_scene_data(*args.primary_cmd);
    //must be rendered after lights are updated 
    shadow_manager->render_shadows(*args.primary_cmd);
    
    m_render_pipeline->render(args);
}

} // namespace vke