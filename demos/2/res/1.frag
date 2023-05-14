#version 450 core

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_color;

layout(location = 0) out vec3 f_color;

layout(set = 0, binding = 0) uniform sampler2D textures[];

void main() {

    f_color = v_color;

    f_color = texture(textures[0], v_uv.xy).xyz * v_color;
}
