#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vke{

struct Transform{
    glm::dvec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    glm::mat4 local_model_matrix(const glm::dvec3& pivot_position = {}){
        auto rel_position = glm::vec3(position - pivot_position);
        return glm::translate(glm::mat4(1), rel_position);
    }
};


}