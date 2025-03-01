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
    vec4 start;
    vec4 size;
};

struct ViewData {
    mat4 proj_view;
    mat4 inv_proj_view;
    dvec4 view_world_pos;
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
    vec4 size;
    vec4 rotation;
    uint model_id;
    uint padd[3];
};

#define MAX_PARTS 32

struct PartData {
    uint mesh_id;
    uint material_id;
};

struct ModelData {
    vec4 aabb_size;
    uint part_index;
    uint part_count;
    uint padd[2];
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

struct PointLight {
    vec4 color;
    vec3 pos;
    float range;
};

struct DirectionalLight {
    vec4 dir;
    vec4 color;
};

// requires std430
struct SceneLightData {
    PointLight point_lights[MAX_LIGHTS];
    DirectionalLight directional_light;
    vec4 ambient_light;
    uint point_light_count;
};

#endif