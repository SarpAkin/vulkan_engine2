#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace vke {

void decompose_matrix(const glm::mat4& matrix, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);

struct Transform {
    glm::dvec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    glm::mat4 local_model_matrix(const glm::dvec3& pivot_position = {});

    static Transform decompose_from_matrix(const glm::mat4& mat);
};

} // namespace vke