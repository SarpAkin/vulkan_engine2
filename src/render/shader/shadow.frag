#version 450

#include "material_set.glsl"
#include "scene_set.glsl"

layout(location = 0) in vec3 f_color;
layout(location = 1) in vec2 f_texture_coord;
layout(location = 2) in vec3 f_normal;
layout(location = 3) in vec3 f_position;

void main() {
#ifdef ALPHA_DISCARD
    float alpha = texture(textures[0], f_texture_coord);
    if (alpha < 0.5) {
        discard;
    }

#endif
}
