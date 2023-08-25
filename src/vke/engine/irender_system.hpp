#pragma once

#include "irender_target.hpp"

namespace vke {

class IRenderSystem {
public:
    virtual void render(vke::IRenderTarget* render_target) = 0;

    virtual void on_subscribe(vke::IRenderTarget*){};
};

} // namespace vke
