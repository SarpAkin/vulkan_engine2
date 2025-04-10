#pragma once

#include <memory>
#include <unordered_map>
#include <vke/fwd.hpp>

#include <entt/fwd.hpp>

#include "fwd.hpp"
#include "render/iobject_renderer.hpp"

namespace vke {

class LightBuffersManager {
public: // c'tor & d'tor
    LightBuffersManager(entt::registry* registry);
    ~LightBuffersManager();

public: // getters
public:
    using LightID = impl::GenericID<CPointLight>;
    void flush_pending_lights(vke::CommandBuffer& cmd);

    IBuffer* get_get_lights_buffer() const { return m_light_buffer.get(); }

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

    std::vector<entt::entity> m_pending_entities_for_register;
    std::vector<LightID> m_pending_instances_for_destruction;

    GenericIDManager<LightID> m_light_id_manager;

    u32 m_max_point_light_count = 1020;
};

} // namespace vke