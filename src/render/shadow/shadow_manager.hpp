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
    const std::vector<float>& get_min_zs_for_direct_shadow_maps(u32 direct_light_index) const { return m_min_z_for_csm; }

    void update_direct_cascaded_shadows(u32 index, glm::vec3 direction);

    VkSampler get_shadow_sampler() { return m_shadow_sampler; }

private:
    void debug_menu();
    void debug_draw_frustums();

private:
    RenderServer* m_render_server;
    VkSampler m_shadow_sampler = VK_NULL_HANDLE;

    std::vector<float> m_min_z_for_csm;

    std::unique_ptr<vke::IShadowMap> m_shadow_map;
    bool m_debug_draw_frustums = false;
    bool m_debug_menu_enabled  = true;
    bool m_update_proj_view    = true;

    float m_csm_multiple_constant = 3.f;
    float m_max_shadow_distance   = 500.f;
    u32 m_direct_shadow_map_count = 4;
};

} // namespace vke