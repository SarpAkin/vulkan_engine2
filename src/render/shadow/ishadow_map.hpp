#pragma once

#include <glm/mat4x4.hpp>
#include <span>
#include <string>
#include <vke/fwd.hpp>

namespace vke {

constexpr std::string shadowD16 = "vke::shadowD16";

enum class ShadowMapType {
    NONE   = 0,
    DIRECT = 1,
    SPOT   = 2,
};

struct ShadowMapCameraData {
    glm::dvec3 position;
    glm::vec3 direction, up; // ignore for point lights
    float far, width, height;
};

class IShadowMap {
public:
    virtual ~IShadowMap() = default;

    virtual ShadowMapType get_shadow_map_type()                                       = 0;
    virtual RCResource<IImageView> get_image_view(bool arrayed = true, u32 index = 0) = 0;
    virtual u32 get_shadow_map_array_size()                                           = 0;
    virtual glm::mat4 get_projection_view_matrix(u32 index = 0, u32 view_index = 0)   = 0;

    virtual glm::dvec3 get_camera_position(u32 index = 0)                     = 0;
    virtual glm::vec3 get_camera_direction(u32 index = 0, u32 view_index = 0) = 0;

    // passed command buffer should be a primary command buffer
    virtual void render(vke::CommandBuffer& primary_cmd, u32 index = 0) = 0;

    virtual void set_camera_data(const ShadowMapCameraData& camera_data, u32 index = 0) = 0;
};

} // namespace vke