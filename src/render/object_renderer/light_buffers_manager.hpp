#pragma once

#include <memory>
#include <unordered_map>
#include <vke/fwd.hpp>

#include <flecs.h>

#include "fwd.hpp"
#include "render/iobject_renderer.hpp"

namespace vke {

class LightBuffersManager {
public: // c'tor & d'tor
    LightBuffersManager(vke::RenderServer* render_server,entt::registry* registry);
    ~LightBuffersManager();

public: // getters
public:
    using LightID = impl::GenericID<CPointLight>;
    void flush_pending_lights(vke::CommandBuffer& cmd);

    IBuffer* get_get_lights_buffer() const { return m_light_buffer.get(); }
    vke::ShadowManager* get_shadow_manager() { return m_shadow_manager.get(); }

private:
    void connect_registry_callbacks();
    void disconnect_registry_callbacks();

    void renderable_component_creation_callback(entt::registry&, entt::entity e);
    void renderable_component_update_callback(entt::registry&, entt::entity e);
    void renderable_component_destroy_callback(entt::registry&, entt::entity e);

private:
    std::unique_ptr<vke::Buffer> m_light_buffer;
    std::unordered_map<std::string, std::unique_ptr<vke::Buffer>> m_light_tile_buffers;
    entt::registry* m_registry;
    vke::RenderServer* m_render_server;
    std::unique_ptr<vke::ShadowManager> m_shadow_manager;

    std::vector<entt::entity> m_pending_entities_for_register;
    std::vector<LightID> m_pending_instances_for_destruction;

    GenericIDManager<LightID> m_light_id_manager = GenericIDManager<LightID>(0);

    u32 m_max_point_light_count = 1020;
};

} // namespace vke