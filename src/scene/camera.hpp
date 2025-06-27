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

    const glm::mat4& proj_view() const { return m_proj_view; }
    const glm::mat4& projection() const { return m_proj; }
    const glm::mat4& view() const { return m_view; }
    const glm::dvec3& get_world_pos() const { return m_world_position; }
    void set_world_pos(const glm::dvec3& world_pos);

    void set_rotation(glm::quat rotation) { m_rotation = rotation; }
    void set_rotation(glm::vec3 forward, glm::vec3 up = {0, 1, 0}) { m_rotation = glm::quatLookAtLH(forward, up); }
    void set_rotation_euler(float pitch, float yaw) { m_rotation = glm::quat(glm::vec3(pitch, yaw, 0)); }
    void set_rotation_euler(const glm::vec3& euler_rotation) { m_rotation = glm::quat(euler_rotation); }

    glm::vec3 get_rotation_euler() const { return glm::eulerAngles(m_rotation); }
    glm::quat get_rotation() const { return m_rotation; }

    glm::vec3 forward() const { return m_rotation * glm::vec3(0, 0, 1); };
    glm::vec3 right() const { return m_rotation * glm::vec3(1, 0, 0); };
    glm::vec3 up() const { return m_rotation * glm::vec3(0, 1, 0); };

    virtual void update();

protected:
    // updates the view matrix from m_rotation and m_world_position
    void update_view();
    virtual void update_proj() = 0;

protected:
    glm::quat m_rotation        = {};
    glm::dvec3 m_world_position = {0, 0, 0};

    glm::mat4 m_proj, m_view, m_proj_view, m_inv_proj_view;
};

class PerspectiveCamera : public Camera {
public:
    ~PerspectiveCamera() {}
    void update_proj() override;

public:
    float fov_deg = 80.0, aspect_ratio = 1.0, z_near = 0.01, z_far = 15000.0;
};

class FreeCamera : public PerspectiveCamera {
public:
    void move_freecam(Window* window, float delta_time);

    ~FreeCamera() {}

private:
    // pitch and yaw in degrees
    float m_pitch_d, m_yaw_d;
};

class OrthographicCamera : public Camera {
public:
    void update_proj() override;

    ~OrthographicCamera() {}

public:
    float half_width, half_height, z_near, z_far;
};

} // namespace vke