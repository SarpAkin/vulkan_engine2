#ifndef SCENE_SET_H
#define SCENE_SET_H

#include "scene_data.h"

#define SCENE_SET 0

layout(set = SCENE_SET, binding = 0) uniform Buffer {
    SceneData scene;
};

#endif