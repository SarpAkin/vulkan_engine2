#ifndef VKE_CULL_UTIL
#define VKE_CULL_UTIL

#include <vke/sets/scene_data.h>
#include <vke/util/quat_util.glsl>

float plane_sdf(vec4 plane, vec3 point) { return dot(plane.xyz, point) - plane.w; }

bool is_visible(in ViewData view,in sampler2D _hzb, in AABB boundary, in vec3 position, in vec4 rotation, in vec3 size) {

    vec3 center = quat_rotate(rotation, boundary.center_point * size) + position;

    // Compute the transformed OBB axes
    vec3 right   = quat_rotate(rotation, vec3(1.0, 0.0, 0.0)) * boundary.half_size.x * size.x;
    vec3 up      = quat_rotate(rotation, vec3(0.0, 1.0, 0.0)) * boundary.half_size.y * size.y;
    vec3 forward = quat_rotate(rotation, vec3(0.0, 0.0, 1.0)) * boundary.half_size.z * size.z;

    for (int i = 0; i < 6; i++) { // Check all 6 frustum planes
        vec4 plane = view.frustum.planes[i];

        float center_distance = plane_sdf(plane, center);

        // Correct projected radius (support mapping)
        float extend_distance = abs(dot(plane.xyz, right)) + abs(dot(plane.xyz, up)) + abs(dot(plane.xyz, forward));

        if (center_distance + extend_distance < 0.0) {
            return false;
        }
    }

    // return true;

#pragma region hzb culling
    if (view.is_hzb_culling_enabled.x != 1) return true;

    vec4 c_center  = view.old_proj_view * vec4(center, 1.0);
    vec4 c_right   = view.old_proj_view * vec4(right, 0.0);
    vec4 c_up      = view.old_proj_view * vec4(up, 0.0);
    vec4 c_forward = view.old_proj_view * vec4(forward, 0.0);

    // it only has to be outside of (-1,1)
    vec3 clip_min = vec3(1E10);
    vec3 clip_max = -clip_min;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                vec4 sum = c_center;
                sum += i == 0 ? -c_right : c_right;
                sum += j == 0 ? -c_up : c_up;
                sum += k == 0 ? -c_forward : c_forward;

                vec3 c_pos = sum.xyz / sum.w;
                clip_min   = min(clip_min, c_pos);
                clip_max   = max(clip_max, c_pos);
            }
        }
    }

    clip_min.xy = clip_min.xy * 0.5 + 0.5;
    clip_max.xy = clip_max.xy * 0.5 + 0.5;

    vec3 clip_size   = (clip_max - clip_min);
    vec3 clip_center = (clip_max + clip_min) * 0.5;

    vec2 hzb_texture_size = textureSize(_hzb, 0);

    float level = floor(log2(max(clip_size.x * hzb_texture_size.x, clip_size.y * hzb_texture_size.y)));

    float depth = textureLod(_hzb, clip_center.xy, level).x;

    // 1 is closer to screen while 0 is far
    return clip_max.z >= depth;
}

#endif