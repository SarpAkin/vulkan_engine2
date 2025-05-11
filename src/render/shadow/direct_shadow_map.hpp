#pragma once

#include <vke/vke.hpp>

#include <memory>

#include "fwd.hpp"
#include "ishadow_map.hpp"

namespace vke {

class DirectShadowMap : public IShadowMap {
public:
    DirectShadowMap(RenderServer* render_server, u32 texture_size = 1024, u32 layers = 1);

    ~DirectShadowMap();

    ShadowMapType get_shadow_map_type() override { return ShadowMapType::DIRECT; }
    RCResource<IImageView> get_image_view(bool, u32) override { return m_shadow_map; }
    u32 get_shadow_map_array_size() override { return m_layer_count; }
    glm::mat4 get_projection_view_matrix(u32, u32) override;
    void set_camera_data(const ShadowMapCameraData& camera_data, u32) override;

    glm::dvec3 get_camera_position(u32 index = 0) override;
    glm::vec3 get_camera_direction(u32 index = 0, u32 view_index = 0) override;

    void render(vke::CommandBuffer& primary_buffer, u32,std::vector<LateRasterData>* raster_buffers) override;


    bool requires_rerender(u32 index)const override{return m_shadow_maps_waiting_for_rerender[index];}

private:
    vke::RCResource<vke::IImageView> m_shadow_map;
    std::unique_ptr<vke::Renderpass> m_shadow_pass;
    RenderServer* m_render_server;
    ObjectRenderer* m_object_renderer;
    std::vector<std::string> m_render_target_names;
    std::vector<std::unique_ptr<vke::OrthographicCamera>> m_cameras;
    std::vector<bool> m_shadow_maps_waiting_for_rerender;
    u32 m_layer_count = 0;
};

} // namespace vke