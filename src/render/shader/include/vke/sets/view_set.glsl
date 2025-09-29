#ifndef VIEW_SET_H
#define VIEW_SET_H

#include "scene_data.h"
#include <vke/util/cull_util.glsl>

#extension GL_EXT_scalar_block_layout : require

#ifndef VIEW_SET
#define VIEW_SET 0
#endif

layout(set = VIEW_SET, binding = 0, std430) uniform BufferV0_SceneView {
    ViewData scene_view;
};

layout(set = VIEW_SET, binding = 1) uniform sampler2D hzb;

// overload of is_visible for the local scene set
bool is_visible(in AABB boundary, in vec3 position, in vec4 rotation, in vec3 size) {
    return is_visible(scene_view, hzb, boundary, position, rotation, size);
}

#endif