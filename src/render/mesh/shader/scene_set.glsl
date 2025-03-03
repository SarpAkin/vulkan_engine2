#ifndef SCENE_SET_H
#define SCENE_SET_H

#include "scene_data.h"

#extension GL_EXT_scalar_block_layout : require

#define SCENE_SET 0
#define VIEW_SET 1

layout(set = SCENE_SET, binding = 0, std430) readonly buffer BufferS0 {
    SceneLightData lights;
};

layout(set = SCENE_SET, binding = 1, std430) readonly buffer BufferS1 {
    InstanceData instances[];
};

layout(set = SCENE_SET, binding = 2, std430) readonly buffer BufferS2 {
    ModelData models[];
};

layout(set = SCENE_SET, binding = 3, std430)readonly buffer BufferS3 {
    PartData parts[];
};

layout(set = SCENE_SET,binding = 4,std430) readonly buffer BufferS4{
    MeshData meshes[];
};

#ifdef COMPUTE

// layout(set = SCENE_SET,binding )

#endif

layout(set = VIEW_SET, binding = 0, std430) uniform BufferV0 {
    ViewData scene_view;
};

layout(set = VIEW_SET, binding = 1, std430) readonly buffer BufferV1 {
    //indexes correspond to partIDs
    uvec2 instance_draw_parameter_locations[];
};

layout(set = VIEW_SET, binding = 2, std430) buffer BufferV2 {
    //indexes correspond to partIDs
    uint instance_counters[];
};

layout(set = VIEW_SET, binding = 3, std430) readonly buffer BufferV3 {
    //indexes correspond to partIDs
    uint indirect_draw_locations[];
};

layout(set = VIEW_SET, binding = 4, std430) buffer BufferV4 {
    //indexes should come from indirect_draw_locations[partID]
    VkDrawIndexedIndirectCommand_ draw_commands[];
};

layout(set = VIEW_SET, binding = 5, std430) buffer BufferV5 {
    InstanceDrawParameter instance_draw_parameters[];
};

#endif
