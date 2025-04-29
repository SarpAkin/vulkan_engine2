#pragma once

#include "fwd.hpp"

#include <memory>
#include <vke/fwd.hpp>
#include <vulkan/vulkan_core.h>

#include "ishadow_map.hpp"

namespace vke {

class ShadowManager {
public:
    ShadowManager(RenderServer* render_server);
    ~ShadowManager();

    void render_shadows(vke::CommandBuffer& cmd);
    void set_direct_shadow_map_camera(u32 index, const ShadowMapCameraData& data) {
        assert(index == 0);
        m_shadow_map->set_camera_data(data);
    }

    vke::RCResource<IImageView> get_direct_shadow_map_texture(u32 index);
    IShadowMap* get_direct_shadow_map(u32 index) { return m_shadow_map.get(); }

    VkSampler get_shadow_sampler() { return m_shadow_sampler; }
private:
    void debug_menu();
    void debug_draw_frustums();

private:
    RenderServer* m_render_server;
    VkSampler m_shadow_sampler = VK_NULL_HANDLE;

    std::unique_ptr<vke::IShadowMap> m_shadow_map;
    bool m_debug_draw_frustums = true;
    bool m_debug_menu_enabled = true;
};

} // namespace vke