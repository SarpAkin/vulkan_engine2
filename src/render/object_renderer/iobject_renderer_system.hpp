#pragma once

#include <vke/fwd.hpp>

#include "../iobject_renderer.hpp"

#include <string>
namespace vke {

class IObjectRendererSystem {
public:
    virtual void register_render_target(const std::string& render_target_name);
    virtual void render(RenderArguments&);

    virtual void update(vke::CommandBuffer& cmd){}
private:
};

} // namespace vke