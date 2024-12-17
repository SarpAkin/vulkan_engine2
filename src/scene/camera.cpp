#include "camera.hpp"

namespace vke {

void Camera::set_world_pos(const glm::dvec3& wpos) {
    m_position = wpos;

    update();
}

void Camera::update() {
    float yaw_sin   = std::sin(glm::radians(m_yaw));
    float yaw_cos   = std::cos(glm::radians(m_yaw));
    float pitch_sin = std::sin(glm::radians(m_pitch));
    float pitch_cos = std::cos(glm::radians(m_pitch));

    pitch_cos = std::max(0.001f, pitch_cos);

    // z+ forward on yaw 0
    m_forward = glm::vec3(
        yaw_cos * pitch_cos,
        pitch_sin,
        yaw_sin * pitch_cos //
    );

    glm::vec3 local_pos = static_cast<glm::vec3>(m_position);

    m_view = glm::lookAt(local_pos, local_pos + m_forward, UP);

    m_proj_view = m_proj * m_view;
}
} // namespace vke