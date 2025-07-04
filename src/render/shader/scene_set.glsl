#ifndef SCENE_SET_H
#define SCENE_SET_H

#include "scene_data.h"

#extension GL_EXT_scalar_block_layout : require

#ifndef SCENE_SET
#define SCENE_SET 1
#endif

#ifndef COMPUTE_SHADER
#define IF_NOT_COMPUTE(x) x
#else
#define IF_NOT_COMPUTE(x)
#endif


layout(set = SCENE_SET, binding = 0, std430) readonly buffer BufferS1_Instances {
    InstanceData instances[];
};

layout(set = SCENE_SET, binding = 1, std430) IF_NOT_COMPUTE(readonly) buffer BufferV5_instance_draw_parameters {
    InstanceDrawParameter instance_draw_parameters[];
};

layout(set = SCENE_SET, binding = 2, std430) readonly buffer BufferV1_InstanceDrawParameterLocations {
    // indexes correspond to partIDs
    uvec2 instance_draw_parameter_locations[];
};

layout(set = SCENE_SET, binding = 3, std430) IF_NOT_COMPUTE(readonly) buffer BufferV2_InstanceCounters {
    // indexes correspond to partIDs
    uint instance_counters[];
};

layout(set = SCENE_SET, binding = 4, std430) readonly buffer BufferV3_IndirectDrawLocations {
    // indexes correspond to partIDs
    uint indirect_draw_locations[];
};

layout(set = SCENE_SET, binding = 5, std430) IF_NOT_COMPUTE(readonly) buffer BufferV4_draw_commands {
    // indexes should come from indirect_draw_locations[partID]
    VkDrawIndexedIndirectCommand_ draw_commands[];
};

layout(set = SCENE_SET, binding = 6, std430) readonly buffer BufferS2_ModelData {
    ModelData models[];
};

layout(set = SCENE_SET, binding = 7, std430) readonly buffer BufferS3_PartData {
    PartData parts[];
};

layout(set = SCENE_SET, binding = 8, std430) readonly buffer BufferS4_MeshData {
    MeshData meshes[];
};



#endif