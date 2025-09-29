#version 450

#include <vke/sets/material_set.glsl>
#include <vke/sets/scene_set.glsl>
#include <vke/sets/view_set.glsl>

#include <vke/fs_io/default.glsl>
#include <vke/fs_output/default.glsl>

void main() {
#ifdef ALPHA_DISCARD
    float alpha = texture(textures[0], f_texture_coord);
    if (alpha < 0.5) {
        discard;
    }

#endif
}
