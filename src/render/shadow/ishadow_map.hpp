#pragma once

#include <glm/mat4x4.hpp>
#include <span>
#include <string>
#include <vke/fwd.hpp>

namespace vke {

constexpr std::string shadowD16 = "shadowD16";

enum class ShadowMapType {
    NONE   = 0,
    DIRECT = 1,
    SPOT   = 2,
};

class IShadowMap {
public:
    virtual ~IShadowMap() = default;

    virtual ShadowMapType get_shadow_map_type()    = 0;
    virtual IImageView* get_image_view()           = 0;
    virtual u32 get_shadow_map_array_size()        = 0;
    virtual glm::mat4 get_projection_view_matrix() = 0;
};

} // namespace vke