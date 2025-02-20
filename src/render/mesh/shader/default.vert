#version 460

#include "scene_set.glsl"

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_texture_coords;
layout(location = 2) in vec3 v_normal;

// layout(location = 1) in uint v_color;

layout(location = 0) out vec3 fcolor;
layout(location = 1) out vec2 f_uvs;
layout(location = 2) out vec3 f_normal;
layout(location = 3) out vec3 f_position;


layout(push_constant) uniform PC {
    mat4 model_matrix;
    mat4 normal_matrix;
};

void main() {
    vec4 world_position = model_matrix * vec4(v_pos, 1.0);
    gl_Position = scene_view.proj_view * vec4(world_position);
    f_position = world_position.xyz / world_position.w;
    // gl_Position.x = -gl_Position.x;

    f_uvs = v_texture_coords;
    // f_uvs.x = 1.0 - f_uvs.x;
    fcolor = vec3(1.0);

    f_normal = normalize(mat3(normal_matrix) * v_normal);
    // f_normal = v_normal;
    // fcolor = unpackUnorm4x8(v_color).rgb;
}