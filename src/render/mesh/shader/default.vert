#version 460

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_texture_coords;

// layout(location = 1) in uint v_color;

layout(location = 0) out vec3 fcolor;
layout(location = 1) out vec2 f_uvs;


layout(push_constant) uniform PC {
    mat4 mvp;
};

void main() {
    gl_Position = mvp * vec4(v_pos, 1.0);

    f_uvs = v_texture_coords;
    fcolor = vec3(1.0);

    // fcolor = unpackUnorm4x8(v_color).rgb;
}