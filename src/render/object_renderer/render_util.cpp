#include "render_util.hpp"

namespace vke {

glm::vec4 calculate_plane_of_triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 ab     = b - a;
    glm::vec3 ac     = c - a;
    glm::vec3 normal = glm::normalize(glm::cross(ab, ac));

    return glm::vec4(normal, glm::dot(normal, a));
}

Frustum calculate_frustum(const glm::mat4& ipv, bool reverse_z) {
    glm::vec3 boundaries[2][2][2];

    for (int x = 0; x < 2; ++x) {
        float xf = (x - 0.5f) * 2.f;

        for (int y = 0; y < 2; ++y) {
            float yf = (y - 0.5f) * 2.f;

            for (int z = 0; z < 2; ++z) {
                glm::vec4 res       = ipv * glm::vec4(xf, yf, float(z), 1);
                boundaries[x][y][z] = glm::vec3(res) / res.w;
            }
        }
    }

    Frustum frustum;

    // planes are tested in the order from 0-5 so we put the planes that are likely to cull most at the start
    frustum.planes[0] = calculate_plane_of_triangle(boundaries[0][0][0], boundaries[0][1][0], boundaries[1][1][0]); // near

    frustum.planes[1] = calculate_plane_of_triangle(boundaries[1][1][0], boundaries[1][0][1], boundaries[1][0][0]); // right
    frustum.planes[2] = calculate_plane_of_triangle(boundaries[0][0][1], boundaries[0][1][0], boundaries[0][0][0]); // left
    frustum.planes[3] = calculate_plane_of_triangle(boundaries[0][0][1], boundaries[0][0][0], boundaries[1][0][0]); // bottom
    frustum.planes[4] = calculate_plane_of_triangle(boundaries[1][1][0], boundaries[0][1][0], boundaries[0][1][1]); // top

    frustum.planes[5] = calculate_plane_of_triangle(boundaries[0][0][1], boundaries[1][1][1], boundaries[0][1][1]); // far

    // if reverse z is enabled flip all planes
    if (reverse_z) {
        for (auto& p : frustum.planes) {
            p = -p;
        }
    }

    return frustum;
}

glm::vec4 construct_plane(const glm::vec3& point, const glm::vec3& direction) {
    assert(std::abs(direction.length() - 1.0) < 0.02);

    return glm::vec4(direction, glm::dot(direction, point));
}

} // namespace vke