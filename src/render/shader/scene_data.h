#ifndef SCENE_DATA_H
#define SCENE_DATA_H

#ifdef __cplusplus
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

using namespace glm;
#else
#extension GL_ARB_gpu_shader_int64 : enable

#endif

// copy of VkDrawIndexedIndirectCommand defined in "vulkan.h"
struct VkDrawIndexedIndirectCommand_ {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};

struct AABB {
    vec3 center_point;
    vec3 half_size;
};

struct Frustum{
    vec4 planes[6];
};

struct ViewData {
    mat4 proj_view;
    mat4 inv_proj_view;
    mat4 old_proj_view;
    dvec4 view_world_pos;
    Frustum frustum;
    uvec4 is_hzb_culling_enabled;
    vec4 frame_times; //x is delta y is the running time of the game
};

struct MaterialData {
    uint texture_ids[4];
    float roughness;
    float specular;
    float metallic;
    float padd;
};

struct InstanceData {
    dvec4 world_position;
    vec4 rotation;
    vec3 size;
    uint model_id;
};

#define MAX_PARTS 32

struct PartData {
    uint mesh_id;
    uint material_id;
};

struct ModelData {
    vec3 aabb_half_size;
    uint part_index;
    vec3 aabb_offset;
    uint part_count;
};

struct MeshData {
    uint64_t vertex_pointers[3];
    uint index_offset;
    uint index_count;
};

struct InstanceDrawParameter{
    mat4 model_matrix;
};

#define MAX_LIGHTS 15
#define MAX_SHADOW_CASCADES 4

struct PointLight {
    vec4 color;
    vec3 pos;
    float range;
};

struct DirectionalLight {
    vec4 dir;
    vec4 color;
    mat4 proj_view[MAX_SHADOW_CASCADES];
    float min_zs_for_cascades[4];
};

// requires std430
struct SceneLightData {
    DirectionalLight directional_light;
    vec4 ambient_light;
    uint point_light_count;
    uint padd[3];
};

#endif