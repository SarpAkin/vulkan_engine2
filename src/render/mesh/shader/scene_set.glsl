#ifndef SCENE_SET_H
#define SCENE_SET_H

#include "scene_data.h"

#extension GL_EXT_scalar_block_layout : require

#define SCENE_SET 0
#define VIEW_SET 1

layout(set = SCENE_SET, binding = 0,std430) readonly buffer Buffer {
    SceneLightData lights;
};


layout(set = VIEW_SET,binding = 0,std430) uniform Buffer1{
    ViewData scene_view;
};


#endif