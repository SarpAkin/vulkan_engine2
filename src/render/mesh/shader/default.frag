#version 450

#define MATERIAL_SET 0

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 texture_coord;

layout(location = 0) out vec4 o_color;

layout(set = MATERIAL_SET, binding = 0) uniform sampler2D textures[4];

void main() {
    o_color = vec4(texture(textures[0], texture_coord).xyz, 1);
}