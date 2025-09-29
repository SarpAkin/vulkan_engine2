#version 450 core

layout(location = 0) in vec3 f_color;

#include <vke/fs_output/auto.glsl>

void main() {
    o_albedo = vec4(f_color, 1.0);
    o_normal = vec4(0.0);
}