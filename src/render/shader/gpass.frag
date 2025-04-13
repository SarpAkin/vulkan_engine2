#version 450

#include "material_set.glsl"
#include "scene_set.glsl"

layout(location = 0) in vec3 f_color;
layout(location = 1) in vec2 f_texture_coord;
layout(location = 2) in vec3 f_normal;
layout(location = 3) in vec3 f_position;

layout(location = 0) out vec4 o_albedo;
layout(location = 1) out vec4 o_normal;

void main() {
    vec3 albedo = texture(textures[0], f_texture_coord).xyz;

    o_albedo = vec4(albedo, 1.0);
    o_normal = vec4(f_normal, 0.0);
}
