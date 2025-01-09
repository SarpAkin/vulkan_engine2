#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace vke {

class Camera {
public:
    constexpr static glm::vec3 UP = glm::vec3(0, 1, 0);

    const glm::mat4& proj_view() const { return m_proj_view; }
    const glm::mat4& projection() const { return m_proj; }
    const glm::mat4& view() const { return m_view; }
    const glm::dvec3& get_world_pos() const { return world_position; }

    void set_world_pos(const glm::dvec3& wpos);


    void update();

public:
    float fov_deg, aspect_ratio, z_near, z_far;
    float pitch, yaw;
    glm::vec3 forward;
    glm::dvec3 world_position;
private:

    glm::mat4 m_proj, m_view, m_proj_view, m_inv_proj_view;
};

} // namespace vke