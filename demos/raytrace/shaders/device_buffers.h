#ifndef _DEVICE_BUFFERS_
#define _DEVICE_BUFFERS_

#ifdef __cplusplus

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

using mat4 = glm::mat4;
using vec4 = glm::vec4;
using vec3 = glm::vec3;
using vec2 = glm::vec2;
using uint = uint32_t;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;
using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

#endif

struct GPUCamera {
    vec3 pos;
    float tan_half_fovx;
    vec3 dir;
    float tan_half_fovy;
    vec3 up;
    float padding;
    vec3 right;
    float padding2;
};

struct RayTracerConfigUBO {
    GPUCamera cam;
    mat4 inv_proj_view;
    vec2 screen_pixel_size;
    vec2 padding;
    vec3 sun_dir;
    float ambient_lightning;
    vec3 sun_color;
    float padding2;
};

struct Sphere {
    vec3 pos;
    float radius;
    vec3 color;
    float roughness;
};

struct AABB {
    vec3 pos;
    float padding;
    vec3 size;
    float padding2;
    vec3 color;
    uint filled_bits;
};

const uint OBJECT_BUFFER_MAX_SPHERES = 15;
const uint OBJECT_BUFFER_MAX_AABB    = 8;
const uint SUB_GROUB_SIZE_XY         = 8;

struct ObjectBuffer {
    uint sphere_count;
    uint box_count;
    uint padding[2];
    Sphere spheres[OBJECT_BUFFER_MAX_SPHERES];
    AABB boxes[OBJECT_BUFFER_MAX_AABB];
};

#endif