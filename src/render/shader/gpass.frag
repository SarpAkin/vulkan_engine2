#version 450

#include <vke/sets/material_set.glsl>
#include <vke/sets/scene_set.glsl>
#include <vke/sets/view_set.glsl>

#include <vke/fs_io/default.glsl>
#include <vke/fs_output/gpass.glsl>

void main() {
    vec3 albedo = texture(textures[0], f_uvs).xyz;

    o_albedo = vec4(albedo, 1.0);
    o_normal = vec4(f_normal, 0.0);
}