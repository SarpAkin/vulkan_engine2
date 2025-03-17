#pragma once

#include "render/shader/scene_data.h"

namespace vke{

Frustum calculate_frustum(const glm::mat4& inv_proj_view);

//direction should be nprmalized
glm::vec4 construct_plane(const glm::vec3& point, const glm::vec3& direction);
}