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

glm::mat4 calculate_optimal_direct_shadow_map_frustum(const glm::mat4& inv_proj_view, float z_s, float z_e, glm::vec3 direct_light_dir, vke::LineDrawer* ld) {
    assert(std::abs(glm::length(direct_light_dir) - 1.0) < 0.05 && "direct_light_dir must be normalized");

    glm::vec3 points[8] = {
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

    glm::vec3 initial_up    = {0, 1, 0};
    glm::mat4 inital_shadow = glm::lookAt({0, 0, 0}, direct_light_dir, initial_up);

    // calculate in shadow space
    glm::vec2 shadow_points[8];
    float shadow_z_min = std::numeric_limits<float>::max(), shadow_z_max = std::numeric_limits<float>::min();
    for (int i = 0; i < 8; i++) {
        glm::vec3 p      = translate_by_matrix(inital_shadow, points[i]);
        shadow_points[i] = p;

        shadow_z_max = std::max(shadow_z_max, p.z);
        shadow_z_min = std::min(shadow_z_min, p.z);
    }

    const auto hull_indices = graham_scan(shadow_points);
    const auto hull         = hull_indices | views::transform([&](auto i) { return shadow_points[i]; });

    auto compute_obb = [&](glm::mat2 rot) -> std::tuple<glm::vec2, glm::vec2> {
        glm::vec2 max = {-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};
        glm::vec2 min = -max;

        for (auto p0 : hull) {
            auto p = rot * p0;

            min = glm::min(min, p);
            max = glm::max(max, p);
        }

        return std::tuple(min, max);
    };

    float smallest_obb_size = std::numeric_limits<float>::infinity();
    float smallest_obb_rotation;
    glm::vec2 smallest_obb_min, smallest_obb_max;
    glm::vec2 smallest_obb_midpoint;

    for (int i = 0; i < hull.size(); i++) {
        auto p0 = hull[i];
        auto p1 = hull[(i + 1) % hull_indices.size()];

        auto dir = glm::normalize(p1 - p0);

        glm::mat2 m;
        m[0] = dir;
        m[1] = {-dir.y, dir.x};

        auto [min, max] = compute_obb(m);

        auto area_vec = max - min;
        float area    = area_vec.x * area_vec.y;
        assert(area >= 0.f);

        if (area < smallest_obb_size) {
            smallest_obb_size     = area;
            smallest_obb_rotation = std::atan2(m[1].y, m[1].x); // compute the rotation in radians

            smallest_obb_min      = min;
            smallest_obb_max      = max;
            smallest_obb_midpoint = glm::inverse(m) * ((min + max) * 0.5f);
        }
    }

    glm::vec3 up = glm::angleAxis(smallest_obb_rotation, direct_light_dir) * translate_by_matrix(inital_shadow, initial_up);

    auto inv_initial_shadow     = glm::inverse(inital_shadow);
    auto transform_shadow2world = [&](auto p) { return translate_by_matrix(inv_initial_shadow, glm::vec3(p, shadow_z_min)); };

    glm::vec3 eye = transform_shadow2world(smallest_obb_midpoint);

    glm::vec2 half_extend = (smallest_obb_max - smallest_obb_min) * 0.5f;

    glm::mat4 view      = glm::lookAt(eye, eye + direct_light_dir, up);
    glm::mat4 proj      = glm::orthoLH_ZO(-half_extend.x, half_extend.x, -half_extend.y, half_extend.y, 0.f, shadow_z_max - shadow_z_min);
    glm::mat4 proj_view = proj * view;

    if (ld) {
        ld->draw_camera_frustum(glm::inverse(proj_view), 0xFF'FF'FF'FF);
        ld->draw_camera_frustum(inv_proj_view, 0xFF'00'00'FF);

        ld->draw_line(eye, eye + direct_light_dir * 5.f, 0xAA'FF'CC'FF);
        ld->draw_line(eye, eye + up * 3.f, 0xAA'FF'CC'FF);

        auto hull_world = hull | views::transform(transform_shadow2world);
        for (int i = 0; i < hull_world.size(); i++) {
            auto p0 = hull_world[i];
            auto p1 = hull_world[(i + 1) % hull_world.size()];

            ld->draw_line(p0, p1, 0x00'FF'00'FF);
        }

        // ld->draw_line(hull_world[0], transform_shadow2world(smallest_obb_midpoint), 0x00'FF'00'FF);

        // glm::mat2 rotation_matrix;
        // rotation_matrix[0] = {std::cos(-smallest_obb_rotation), std::sin(-smallest_obb_rotation)};
        // rotation_matrix[1] = {-std::sin(-smallest_obb_rotation), std::cos(-smallest_obb_rotation)};

        // auto rotated_shadow2world = [&](auto v) { return transform_shadow2world(rotation_matrix * v); };

        // glm::vec3 extend_points[4] = {
        //     rotated_shadow2world(glm::vec2(smallest_obb_min.x,smallest_obb_min.y)),
        //     rotated_shadow2world(glm::vec2(smallest_obb_min.x,smallest_obb_max.y)),
        //     rotated_shadow2world(glm::vec2(smallest_obb_max.x,smallest_obb_max.y)),
        //     rotated_shadow2world(glm::vec2(smallest_obb_max.x,smallest_obb_min.y)),
        // };
    
        // for(int i = 0;i < 4;i++){
        //     // ld->draw_line(extend_points[i], extend_points[(i + 1) % 4], 0xCC'CC'00'FF);
        // }
    }

    return proj_view;
}

} // namespace vke