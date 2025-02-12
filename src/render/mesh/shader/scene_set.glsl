#ifndef SCENE_SET_H
#define SCENE_SET_H

#include "scene_data.h"

#extension GL_EXT_scalar_block_layout : require

#define SCENE_SET 0

layout(set = SCENE_SET, binding = 0,std430) uniform Buffer {
    SceneData scene;
};

#endif