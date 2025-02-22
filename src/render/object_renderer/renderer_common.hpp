#pragma once

namespace vke {

enum class MaterialSubpassType {
    NONE = 0,
    FORWARD,
    DEFERRED_PBR,
    SHADOW,
    CUSTOM,
};

struct RenderState;

} // namespace vke