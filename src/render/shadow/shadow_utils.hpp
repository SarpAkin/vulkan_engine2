#pragma once

#include <glm/mat4x4.hpp>
#include "glm/vec3.hpp"

#include "fwd.hpp"


namespace vke{

//this function calculates an optimal shadow map for the planes between z_start and z_end
// these values are for in clip space 
glm::mat4 calculate_optimal_direct_shadow_map_frustum(const glm::mat4& inv_proj_view, float z_start, float z_end,glm::vec3 direct_light_dir,vke::LineDrawer* debug_line_drawer = nullptr);


}