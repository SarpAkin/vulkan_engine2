#pragma once

#include <entt/entt.hpp>
#include <vke/util.hpp>

#include "transform.hpp"

namespace vke{

struct CParent{
    entt::entity parent;
};

struct CChilds{
    vke::SmallVec<entt::entity> children;
};

}

