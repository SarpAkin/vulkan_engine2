#version 460

#include <vke/sets/scene_set.glsl>
#include <vke/sets/view_set.glsl>

#include <vke/vs_input/default.glsl>
#include <vke/fs_io/default.glsl>


layout(push_constant) uniform PC {
    mat4 p_model_matrix;
    mat4 p_normal_matrix;
    uint mode;
};

mat4 model_matrix  = mode != 0 ? instance_draw_parameters[gl_InstanceIndex].model_matrix : p_model_matrix;
mat4 normal_matrix = mode != 0 ? model_matrix : p_normal_matrix;

void main() {
    setup_vt_input();

    vec4 world_position = model_matrix * vec4(v_pos, 1.0);
    gl_Position         = scene_view.proj_view * vec4(world_position);

// #ifndef SHADOW_PASS
    f_position = world_position.xyz / world_position.w;
    // gl_Position.x = -gl_Position.x;

    f_uvs = v_texture_coords;
    // f_uvs.x = 1.0 - f_uvs.x;
    f_color = vec3(1.0);

    f_normal = normalize(mat3(normal_matrix) * v_normal);
// f_normal = v_normal;
// f_color = unpackUnorm4x8(v_color).rgb;
// #endif
}