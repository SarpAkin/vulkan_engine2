#version 450 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_color;

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);

    v_uv    = color.xy;
    v_color = color;
}
