#pragma once

#include <memory>
#include <unordered_map>
#include <vke/fwd.hpp>

#include <flecs.h>

#include "fwd.hpp"
#include "render/iobject_renderer.hpp"
#include "generic_entity_gpu_handle_manager.hpp"


namespace vke {

class LightBuffersManager {
public: // c'tor & d'tor
    LightBuffersManager(vke::RenderServer* render_server,flecs::world* world);
    ~LightBuffersManager();

public: // getters
public:
    using LightID = GPUHandleIDManager<CPointLight>::HandleID;
    void flush_pending_lights(vke::CommandBuffer& cmd);

    IBuffer* get_get_lights_buffer() const { return m_light_buffer.get(); }
    vke::ShadowManager* get_shadow_manager() { return m_shadow_manager.get(); }

private:
    std::unique_ptr<vke::Buffer> m_light_buffer;
    std::unordered_map<std::string, std::unique_ptr<vke::Buffer>> m_light_tile_buffers;
    flecs::world* m_world;
    vke::RenderServer* m_render_server;
    std::unique_ptr<vke::ShadowManager> m_shadow_manager;


    std::unique_ptr<GPUHandleIDManager<CPointLight>> m_light_handle_manager;
    

    u32 m_max_point_light_count = 1020;
};

} // namespace vke