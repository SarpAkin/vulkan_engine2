#include "shadow_utils.hpp"

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <ranges>
#include <vke/util.hpp>

#include <span>
#include <vke/fwd.hpp>

#include "render/debug/line_drawer.hpp"

namespace vke {

static glm::vec3 translate_by_matrix(const glm::mat4& m, glm::vec3 p) {
    glm::vec4 p4 = m * glm::vec4(p, 1.0);
    return glm::vec3(p4) / p4.w;
}

static std::vector<u16> graham_scan(std::span<const glm::vec2> points) {
    auto ccw = +[](glm::vec2 a, glm::vec2 b, glm::vec2 c) -> float {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    };

    std::vector<u16> sorted_points;
    u16 smallest_index = 0;
    for (int i = 0; i < points.size(); i++) {
        glm::vec2 s = points[smallest_index];
        glm::vec2 c = points[i];
        if (s.y < c.y) {
            smallest_index = i;
        }
    }

    glm::vec2 p0 = points[smallest_index];

    auto thetas = vke::map_vec(points, [&](glm::vec2 v) {
        auto d = v - p0;
        return std::atan2(d.y, d.x);
    });

    sorted_points.reserve(points.size() - 1);
    for (u16 i = 0; i < smallest_index; i++) {
        sorted_points.push_back(i);
    }
    for (u16 i = smallest_index + 1; i < points.size(); i++) {
        sorted_points.push_back(i);
    }

    std::sort(sorted_points.begin(), sorted_points.end(), [&](u16 a, u16 b) {
        return thetas[a] < thetas[b];
    });

    std::vector<u16> stack = {smallest_index, sorted_points[0]};

    for (u16 i = 1; i < sorted_points.size(); i++) {
        while (stack.size() >= 2 ? ccw(points[stack[stack.size() - 2]], points[stack[stack.size() - 1]], points[sorted_points[i]]) <= 0 : false) {
            stack.pop_back();
        }
        stack.push_back(sorted_points[i]);
    }

    return stack;
}

std::tuple<glm::vec2, glm::vec2> compute_aabb(auto&& points) {
    glm::vec2 max = {-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};
    glm::vec2 min = -max;

    for (auto p : points) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }

    return std::tuple(min, max);
}

struct ShadowOBB {
    glm::vec2 rotated_min, rotated_max;
    glm::vec2 mid_point;
    glm::mat2 rotation_mat;
    float rotation_rad;
};

ShadowOBB find_obb(auto&& hull) {
    float smallest_obb_size = std::numeric_limits<float>::infinity();

    ShadowOBB smallest_obb;

    for (int i = 0; i < hull.size(); i++) {
        auto p0 = hull[i];
        auto p1 = hull[(i + 1) % hull.size()];

        auto dir = glm::normalize(p1 - p0);

        glm::mat2 m;
        m[0] = dir;
        m[1] = {-dir.y, dir.x};

        auto [min, max] = compute_aabb(hull | views::transform([&](auto p) { return m * p; }));

        auto area_vec = max - min;
        float area    = area_vec.x * area_vec.y;
        assert(area >= 0.f);

        if (area < smallest_obb_size) {
            smallest_obb_size = area;

            smallest_obb = {
                .rotated_min  = min,
                .rotated_max  = max,
                .mid_point    = glm::inverse(m) * ((min + max) * 0.5f),
                .rotation_mat = m,
                .rotation_rad = std::atan2(m[0].y, m[0].x), // compute the rotation in radians
            };
        }
    }

    return smallest_obb;
}

std::vector<glm::vec3> calculate_camera_frustum(const glm::mat4& inv_proj_view, float z_s, float z_e) {
    std::vector<glm::vec3> points = {
        {-1, -1, z_s},
        {-1, +1, z_s},
        {+1, -1, z_s},
        {+1, +1, z_s},
        {-1, -1, z_e},
        {-1, +1, z_e},
        {+1, -1, z_e},
        {+1, +1, z_e},
    };

    // transform the points
    for (auto& p : points) {
        p = translate_by_matrix(inv_proj_view, p);
    }

    return points;
}

//calculates the hull in shadow space
std::vector<glm::vec2> calculate_hull(std::span<const glm::vec3> points, const glm::mat4& inital_shadow, float& z_min, float z_max) {

    float shadow_z_min = std::numeric_limits<float>::max(), shadow_z_max = std::numeric_limits<float>::min();

    auto shadow_points = vke::map_vec(points, [&](glm::vec3 p0) {
        glm::vec3 p = translate_by_matrix(inital_shadow, p0);

        shadow_z_max = std::max(shadow_z_max, p.z);
        shadow_z_min = std::min(shadow_z_min, p.z);

        return glm::vec2(p);
    });

    const auto hull_indices = graham_scan(shadow_points);

    z_min = shadow_z_min;
    z_max = shadow_z_max;

    return vke::map_vec(hull_indices, [&](auto i) { return shadow_points[i]; });
}

ShadowMapCameraData calculate_optimal_direct_shadow_map_frustum(const glm::mat4& inv_proj_view, float z_s, float z_e, glm::vec3 direct_light_dir, float shadow_z_far, vke::LineDrawer* ld) {
    assert(std::abs(glm::length(direct_light_dir) - 1.0) < 0.05 && "direct_light_dir must be normalized");

    auto points = calculate_camera_frustum(inv_proj_view, z_s, z_e);

    glm::mat4 inital_shadow = glm::lookAtRH({0, 0, 0}, direct_light_dir, {0, 1, 0});

    // calculate in shadow space
    float shadow_z_min,shadow_z_max;
    auto hull = calculate_hull(points, inital_shadow, shadow_z_min, shadow_z_max);

    auto obb = find_obb(hull);

    auto inv_initial_shadow     = glm::inverse(inital_shadow);
    auto transform_shadow2world = [&](auto p) { return translate_by_matrix(inv_initial_shadow, glm::vec3(p, shadow_z_max)); };

    glm::vec3 eye = transform_shadow2world(obb.mid_point);
    glm::vec3 up  = glm::normalize(transform_shadow2world(obb.mid_point + (glm::inverse(obb.rotation_mat) * glm::vec2(0, 1))) - eye);

    glm::vec2 extend = (obb.rotated_max - obb.rotated_min);

    float far        = shadow_z_max - shadow_z_min;
    float far_excess = shadow_z_far - far;

    eye += direct_light_dir * -far_excess;

    return ShadowMapCameraData{
        .position  = eye,
        .direction = direct_light_dir,
        .up        = up,
        .far       = shadow_z_far,
        .width     = extend.x,
        .height    = extend.y,
    };
}

} // namespace vke