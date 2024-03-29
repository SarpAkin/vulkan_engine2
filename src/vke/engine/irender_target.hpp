#pragma once

#include "../common.hpp"
#include "../fwd.hpp"

namespace vke {

class Camera;
class IRenderSystem;

enum class RenderTargetType {
    DEFERED,
    SHADOW,
    FORWARD,
    FORWARD_TRANSPARENT,
    CUSTOM,
    UNDEFINED,
};

class IRenderTarget {
public:
    virtual ~IRenderTarget(){};

    virtual Renderpass* get_renderpass() = 0;
    virtual u32 get_subpass_index()      = 0;
    virtual Camera* get_camera()         = 0;
    virtual void set_camera(Camera*)     = 0;

    virtual CommandBuffer* get_draw_cmd()    = 0;
    virtual CommandBuffer* get_compute_cmd() = 0;

    virtual void subscribe(IRenderSystem* system) = 0;

    // renders the systems into the commandbuffers;
    virtual void render_systems() = 0;

    virtual RenderTargetType get_render_target_type()const { return RenderTargetType::UNDEFINED; }
};

} // namespace vke