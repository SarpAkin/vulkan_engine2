#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace vke {

class Window;

class Camera {
public:
    Camera() {}
    ~Camera() {}

    constexpr static glm::vec3 UP = glm::vec3(0, 1, 0);

    void move_freecam(Window* window, float delta_time);

    const glm::mat4& proj_view() const { return m_proj_view; }
    const glm::mat4& projection() const { return m_proj; }
    const glm::mat4& view() const { return m_view; }
    const glm::dvec3& get_world_pos() const { return world_position; }

    void set_world_pos(const glm::dvec3& world_pos);

    virtual void update() { assert(0); }

public:
    float pitch = 0, m_pad[20], yaw = 0;
    glm::vec3 forward         = {0, 0, 1};
    glm::dvec3 world_position = {0, 0, 0};

protected:
    glm::mat4 m_proj, m_view, m_proj_view, m_inv_proj_view;
};

class PerspectiveCamera : public Camera {
public:
    ~PerspectiveCamera() {}
    void update() override;

public:
    float fov_deg = 80.0, aspect_ratio = 1.0, z_near = 0.1, z_far = 3000.0;
};

class FreeCamera : public PerspectiveCamera {
public:
    ~FreeCamera() {}
};

class OrthographicCamera : public Camera {
public:
    ~OrthographicCamera() {}
};

} // namespace vke