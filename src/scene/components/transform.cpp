#include "transform.hpp"

namespace vke{

void decompose_matrix(const glm::mat4& matrix, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale) {
    // Extract translation
    translation = glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);

    // Extract scale
    glm::vec3 scaleX = glm::vec3(matrix[0][0], matrix[0][1], matrix[0][2]);
    glm::vec3 scaleY = glm::vec3(matrix[1][0], matrix[1][1], matrix[1][2]);
    glm::vec3 scaleZ = glm::vec3(matrix[2][0], matrix[2][1], matrix[2][2]);

    scale = glm::vec3(glm::length(scaleX), glm::length(scaleY), glm::length(scaleZ));

    // Normalize the rotation matrix
    glm::mat3 rotationMatrix = glm::mat3(matrix);
    if (scale.x != 0.0f) rotationMatrix[0] /= scale.x;
    if (scale.y != 0.0f) rotationMatrix[1] /= scale.y;
    if (scale.z != 0.0f) rotationMatrix[2] /= scale.z;

    // Convert rotation matrix to quaternion
    rotation = glm::quat_cast(rotationMatrix);
}

glm::mat4 Transform::local_model_matrix(const glm::dvec3& pivot_position) {
    auto rel_position = glm::vec3(position - pivot_position);

    glm::mat3 mat(1);
    mat[0][0] = scale.x;
    mat[1][1] = scale.y;
    mat[2][2] = scale.z;

    mat = glm::mat3_cast(rotation) * mat;

    glm::mat4 mat4 = mat;
    mat4[3] = glm::vec4(rel_position,1.0);

    return mat4;
}

Transform Transform::decompose_from_matrix(const glm::mat4& mat) {
    glm::vec3 translation, scale;
    glm::quat rotation;

    decompose_matrix(mat, translation, rotation, scale);

    return Transform{
        .position = translation,
        .rotation = rotation,
        .scale    = scale,
    };
}
} // namespace vke