#pragma once

#include <vke/fwd.hpp>

#include "../iobject_renderer.hpp"

#include <string>
namespace vke {

class IObjectRendererSystem {
public:
    virtual ~IObjectRendererSystem() {}

public:
    virtual void register_render_target(const std::string& render_target_name) = 0;
    virtual void render(RenderArguments&)                                      = 0;

    virtual void update(vke::CommandBuffer& cmd) = 0;
    virtual void set_registry(entt::registry* reg) {}

private:
};

} // namespace vke