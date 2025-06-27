#include "light_buffers_manager.hpp"

#include <vke/vke.hpp>

#include "render/iobject_renderer.hpp"
#include "render/shader/scene_data.h"
#include "render/shadow/shadow_manager.hpp"
#include "scene/components/components.hpp"

namespace vke {

struct CPointLightInstance {
    const LightBuffersManager::LightID id;
};

LightBuffersManager::LightBuffersManager(vke::RenderServer* render_server, flecs::world* world) {
    m_world = world;
    m_render_server       = render_server;
    m_light_buffer        = std::make_unique<vke::Buffer>(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        sizeof(SceneLightData) + sizeof(PointLight) * m_max_point_light_count, false //
    );

    m_shadow_manager = std::make_unique<ShadowManager>(m_render_server);
    m_light_handle_manager = std::make_unique<GPUHandleIDManager<CPointLight>>(world);
}

LightBuffersManager::~LightBuffersManager() {
}

void LightBuffersManager::flush_pending_lights(vke::CommandBuffer& cmd) {
    StencilBuffer stencil;

    auto point_lights = m_light_buffer->subspan(sizeof(SceneLightData));

    m_light_handle_manager->flush_and_register_handles([&](flecs::entity e, LightID id) {
        auto t = e.get<Transform>();
        auto l = e.get<CPointLight>();

        PointLight light{
            .color = glm::vec4(l->color, 0.0),
            .pos   = glm::vec3(t->position),
            .range = l->range,
        };

        stencil.copy_data(point_lights.subspan_item<PointLight>(id.id, 1), &light, 1);
    });

    glm::vec3 directional_light_dir = glm::normalize(glm::vec3(1, -1, 1));

    // m_shadow_manager->set_direct_shadow_map_camera(0, ShadowMapCameraData{
    //     .position = directional_light_dir * -150.f,
    //     .direction = directional_light_dir,
    //     .far = 300.f,
    //     .width = 100.f,
    //     .height = 100.f,
    // });

    m_shadow_manager->update_direct_cascaded_shadows(0, directional_light_dir);

    SceneLightData scene_light_data{
        .directional_light = {
            .dir   = glm::vec4(directional_light_dir, 1.0),
            .color = glm::vec4(1.0, 1.0, 1.0, 1.0),
            // .proj_view = m_shadow_manager->get_direct_shadow_map(0)->get_projection_view_matrix(),
        },
        .ambient_light     = glm::vec4(0.04, 0.03, 0.03, 0.0),
        .point_light_count = static_cast<u32>(m_light_handle_manager->get_id_manager().id_manager.id_count()),
    };

    auto& min_zs = m_shadow_manager->get_min_zs_for_direct_shadow_maps(0);

    for (int i = 0; i < MAX_SHADOW_CASCADES; i++) {
        scene_light_data.directional_light.proj_view[i]           = m_shadow_manager->get_direct_shadow_map(0)->get_projection_view_matrix(i);
        scene_light_data.directional_light.min_zs_for_cascades[i] = min_zs[i];
    }

    stencil.copy_data(m_light_buffer->subspan(0), &scene_light_data, 1);

    stencil.flush_copies(cmd);
}

} // namespace vke