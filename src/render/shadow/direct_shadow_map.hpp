#pragma once

#include <vke/vke.hpp>

#include <memory>

#include "fwd.hpp"
#include "ishadow_map.hpp"

namespace vke {

class DirectShadowMap : public IShadowMap {
public:
    DirectShadowMap(RenderServer* render_server, u32 texture_size = 1024);

    ~DirectShadowMap();

    ShadowMapType get_shadow_map_type() override { return ShadowMapType::DIRECT; }
    IImageView* get_image_view() override { return m_shadow_map.get(); }
    u32 get_shadow_map_array_size() override { return 1; }
    glm::mat4 get_projection_view_matrix() override;

private:
    vke::RCResource<vke::IImageView> m_shadow_map;
    std::unique_ptr<vke::Renderpass> m_shadow_pass;
    std::unique_ptr<vke::Camera> m_camera;
    RenderServer* m_render_server;
    std::string m_render_target_name;
};

} // namespace vke