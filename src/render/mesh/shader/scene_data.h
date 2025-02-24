#ifndef SCENE_DATA_H
#define SCENE_DATA_H

#ifdef __cplusplus
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

using namespace glm;
#else 
#extension GL_ARB_gpu_shader_int64 : enable

#endif

struct ViewData{
    mat4 proj_view;
    mat4 inv_proj_view;
    dvec4 view_world_pos;
};

struct MaterialData{
    uint texture_ids[4];
    float roughness;
    float specular;
    float metallic;
    float padd;
};

struct InstanceData{
    dvec4 world_position;
    vec4 size;
    vec4 rotation;
    uint model_id;
    uint padd[3];
};

struct PartData{
    uint64_t component_addresses[4];
};

struct ModelData{
    vec4 aabb_size;

};

#define MAX_LIGHTS 15

struct PointLight{
    vec4 color;
    vec3 pos;
    float range;
};

struct DirectionalLight{
    vec4 dir;
    vec4 color;
};

//requires std430
struct SceneLightData{
    PointLight point_lights[MAX_LIGHTS];
    DirectionalLight directional_light;
    vec4 ambient_light;
    uint point_light_count;
};



#endif