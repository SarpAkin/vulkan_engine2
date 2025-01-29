#version 450

#include "scene_set.glsl"

#define MATERIAL_SET 1

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 texture_coord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 o_color;

layout(set = MATERIAL_SET, binding = 0) uniform sampler2D textures[4];

void main() {
    o_color = vec4(texture(textures[0], texture_coord).xyz, 1);
    // o_color = vec4(1);

    // o_color = vec4(normal * 0.5 + 0.5, 1);
}