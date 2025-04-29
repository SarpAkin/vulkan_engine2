#include "light_buffers_manager.hpp"

#include <vke/vke.hpp>

#include "render/iobject_renderer.hpp"
#include "render/shader/scene_data.h"
#include "scene/components/components.hpp"
#include "render/shadow/shadow_manager.hpp"

namespace vke {

struct CPointLightInstance {
    const LightBuffersManager::LightID id;
};

LightBuffersManager::LightBuffersManager(vke::RenderServer* render_server, entt::registry* registry) {
    m_registry     = registry;
    m_render_server = render_server;
    m_light_buffer = std::make_unique<vke::Buffer>(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        sizeof(SceneLightData) + sizeof(PointLight) * m_max_point_light_count, false //
    );

    connect_registry_callbacks();

    m_shadow_manager = std::make_unique<ShadowManager>(m_render_server);
}

LightBuffersManager::~LightBuffersManager() {
    disconnect_registry_callbacks();
}

void LightBuffersManager::connect_registry_callbacks() {
    m_registry->on_construct<CPointLight>().connect<&LightBuffersManager::renderable_component_creation_callback>(this);
    m_registry->on_update<CPointLight>().connect<&LightBuffersManager::renderable_component_update_callback>(this);
    m_registry->on_destroy<CPointLight>().connect<&LightBuffersManager::renderable_component_destroy_callback>(this);
}

void LightBuffersManager::disconnect_registry_callbacks() {
    m_registry->on_construct<CPointLight>().disconnect(this);
    m_registry->on_update<CPointLight>().disconnect(this);
    m_registry->on_destroy<CPointLight>().disconnect(this);
}

void LightBuffersManager::renderable_component_creation_callback(entt::registry& r, entt::entity e) {
    m_pending_entities_for_register.push_back(e);
}

void LightBuffersManager::renderable_component_update_callback(entt::registry&, entt::entity e) {
    printf("entity %d's renderable component has been modified\n", e);
    TODO();
}

void LightBuffersManager::renderable_component_destroy_callback(entt::registry& r, entt::entity e) {
    auto instance = r.try_get<CPointLightInstance>(e);
    if (instance == nullptr) return;

    m_pending_instances_for_destruction.push_back(instance->id);

    // m_pending_instances_for_destruction.push_back(instance->id);
}

void LightBuffersManager::flush_pending_lights(vke::CommandBuffer& cmd) {
    StencilBuffer stencil;

    auto view = m_registry->view<Transform, CPointLight>();

    auto point_lights = m_light_buffer->subspan(sizeof(SceneLightData));

    for (auto e : m_pending_entities_for_register) {
        auto id = m_light_id_manager.new_id();

        m_registry->emplace<CPointLightInstance>(e, CPointLightInstance{.id = id});

        auto [t, l] = view.get(e);

        PointLight light{
            .color = glm::vec4(l.color, 0.0),
            .pos   = glm::vec3(t.position),
            .range = l.range,
        };

        stencil.copy_data(point_lights.subspan_item<PointLight>(id.id, 1), &light, 1);
    }

    m_pending_entities_for_register.clear();

    glm::vec3 directional_light_dir = glm::normalize(glm::vec3(1,-1,1));

    m_shadow_manager->set_direct_shadow_map_camera(0, ShadowMapCameraData{
        .position = directional_light_dir * -150.f,
        .direction = directional_light_dir,
        .far = 300.f,
        .width = 100.f,
        .height = 100.f,
    });

    SceneLightData scene_light_data{
        .directional_light = {
            .dir   = glm::vec4(directional_light_dir, 1.0),
            .color = glm::vec4(1.0, 1.0, 1.0, 1.0),
            .proj_view = m_shadow_manager->get_direct_shadow_map(0)->get_projection_view_matrix(),
        },
        .ambient_light     = glm::vec4(),
        .point_light_count = static_cast<u32>(m_light_id_manager.id_manager.id_count()),
    };

    stencil.copy_data(m_light_buffer->subspan(0), &scene_light_data, 1);

    stencil.flush_copies(cmd);
}

} // namespace vke