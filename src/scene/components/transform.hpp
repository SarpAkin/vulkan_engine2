#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace vke {

void decompose_matrix(const glm::mat4& matrix, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);

struct RelativeTransform;

struct Transform {
    glm::dvec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    glm::mat4 local_model_matrix(const glm::dvec3& pivot_position = {}) const;

    static Transform decompose_from_matrix(const glm::mat4& mat);

    Transform operator*(const Transform& t) const;

    // Cast to RelativeTransform
    explicit operator RelativeTransform() const;

    const static Transform IDENTITY;
};

struct RelativeTransform {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    explicit operator Transform() const {
        return Transform{
            glm::dvec3(position),
            rotation,
            scale,
        };
    }

    glm::mat4 get_model_matrix() const{ return static_cast<Transform>(*this).local_model_matrix(); }
    static RelativeTransform decompose_from_matrix(const glm::mat4& mat) { return static_cast<RelativeTransform>(Transform::decompose_from_matrix(mat)); }
};

} // namespace vke