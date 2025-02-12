#ifndef SCENE_DATA_H
#define SCENE_DATA_H

#ifdef __cplusplus
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

using namespace glm;
#endif

struct SceneData{
    mat4 proj_view;
    mat4 inv_proj_view;
    dvec4 view_world_pos;
    vec4 directional_light_dir;
    vec4 directional_light_color;
    vec4 ambient_light;
};

struct MaterialData{
    float roughness;
    float specular;
    float metalic;
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