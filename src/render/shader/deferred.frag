#version 450
#extension GL_KHR_shader_subgroup_quad : enable

#define DEFERRED_SET 0

#define PI 3.141592653589793

#include "scene_data.h"

layout(set = DEFERRED_SET, binding = 0) uniform sampler2D textures[3];

layout(set = DEFERRED_SET, binding = 1) uniform ViewDataBuffer {
    ViewData view;
};

layout(set = DEFERRED_SET, binding = 2) readonly buffer LightsBuffer {
    SceneLightData lights;
    PointLight point_lights[];
};

layout(set = DEFERRED_SET, binding = 3) uniform sampler2DShadow shadow_maps[1];
// layout(set = DEFERRED_SET,binding = 3) uniform samplerCubeArrayShadow shadow_point_maps[1];

float calculate_light_strength(vec3 light_dir, vec3 normal, vec3 view_dir);

vec4 debug_color;

const int poisson_table_size           = 32;
vec2 poisson_table[poisson_table_size] = {
    vec2(0.087164, -0.25865),
    vec2(-0.71380, -0.31938),
    vec2(-0.42212, -0.37691),
    vec2(-0.35237, -0.048855),
    vec2(0.80181, -0.073725),
    vec2(-0.050746, -0.59212),
    vec2(-0.030902, 0.044495),
    vec2(0.11101, -0.17909),
    vec2(-0.11017, -0.0071830),
    vec2(0.57569, 0.42861),
    vec2(-0.33310, 0.83733),
    vec2(0.42835, 0.87701),
    vec2(-0.64454, -0.68726),
    vec2(0.19806, 0.33082),
    vec2(0.048796, 0.24135),
    vec2(0.21022, -0.84828),
    vec2(0.27345, 0.34345),
    vec2(0.42546, -0.084226),
    vec2(-0.56231, 0.17967),
    vec2(0.0056380, -0.11294),
    vec2(-0.16145, -0.076814),
    vec2(-0.029326, -0.017768),
    vec2(0.28798, 0.14798),
    vec2(-0.17497, -0.055957),
    vec2(0.30198, -0.49570),
    vec2(-0.84580, 0.40599),
    vec2(-0.25414, 0.47697),
    vec2(0.98356, -0.13102),
    vec2(-0.10180, 0.032886),
    vec2(0.032977, 0.24390),
    vec2(0.66924, 0.032143),
    vec2(-0.91677, 0.22138),
};

float quad_average(float n) {
    float v = subgroupQuadSwapVertical(n);
    n       = (n + v) * 0.5;

    float h = subgroupQuadSwapHorizontal(n);
    n       = (n + h) * 0.5;

    return n;
}

uint permutate(uint x) {
    x = (x ^ 61) ^ (x >> 16);
    x *= 9;
    x ^= (x >> 4);
    x *= 0x27d4eb2d;
    x ^= (x >> 15);
    return x;
}

vec3 calculate_direct_light(vec4 world_pos4, vec3 world_pos, vec3 normal, vec3 view_dir) {
    // config
    const float normal_offset_value = 0.035;
    const float base_bias           = 0.0001;
#define POISSON_SAMPLING
#define WORLD_SPACE_SEED

    // normal offsetting
    world_pos4.xyz += normal * (normal_offset_value * world_pos4.w);

    vec4 shadow_pos4 = lights.directional_light.proj_view * world_pos4;
    vec3 shadow_pos  = shadow_pos4.xyz / shadow_pos4.w;
    // shadow_pos.z = .0;

    vec2 shadow_uv              = vec2(shadow_pos.xy * 0.5 + 0.5);
    float shadow_map_resolution = 4096;

    shadow_uv = floor(shadow_uv * shadow_map_resolution) / shadow_map_resolution;

    float bias = max((dot(lights.directional_light.dir.xyz, normal)), 0.1) * base_bias;

#ifdef POISSON_SAMPLING

#ifndef WORLD_SPACE_SEED
    uint seed = (uint(gl_FragCoord.x * 7 + 123214) << 17) ^ uint(gl_FragCoord.y * 3 + 1336);
#else
    // uvec3 pos2 = uvec3(ivec3(shadow_pos * (100000)));
    uvec3 pos2 = uvec3(ivec3(world_pos * (10000)));
    uint seed  = permutate(permutate(pos2.x ^ permutate(pos2.y) ^ (gl_SubgroupInvocationID & 3)) ^ (pos2.z));
#endif

    const uint sample_count = 4;
    float is_lit            = 0.0;
    vec2 poisson_mul        = vec2(1.0) / vec2(textureSize(shadow_maps[0], 0));
    for (int i = 0; i < sample_count; i++) {
        seed              = permutate(seed + i);
        vec2 poisson_disc = poisson_table[seed % poisson_table_size];
        is_lit += texture(shadow_maps[0], vec3(shadow_uv + poisson_disc * poisson_mul, shadow_pos.z + bias));
    }

    is_lit /= sample_count;
#else
    float is_lit = texture(shadow_maps[0], vec3(shadow_uv, shadow_pos.z + bias));
#endif

    is_lit = quad_average(is_lit);

    debug_color = vec4(is_lit, 0.0, 0.0, 0.0);

    float light = calculate_light_strength(lights.directional_light.dir.xyz, normal, view_dir);
    light *= max(is_lit, 0.1);

    return light * lights.directional_light.color.xyz;
}

vec3 calculate_total_light(vec4 world_pos4, vec3 world_pos, vec3 normal, vec3 view_dir) {
    vec3 total_light = vec3(0);

    // calculate point lights
    for (int i = 0; i < min(MAX_LIGHTS, lights.point_light_count); i++) {
        PointLight p   = point_lights[i];
        vec3 light_dir = world_pos - p.pos;
        float d        = length(light_dir);
        light_dir /= d;
        // if light is in range calculate it and add it to total
        if (d < p.range) {
            float strength = calculate_light_strength(light_dir, normal, view_dir);
            float mul      = 1.0 - (d / p.range);
            strength *= mul * mul;
            total_light += p.color.xyz * strength;

            // total_light += normalize(light_dir);
        }
    }

    // calculate directional light
    total_light += calculate_direct_light(world_pos4, world_pos, normal, view_dir);

    // add ambient light
    // total_light += lights.ambient_light.xyz;
    total_light += 0.03;

    return total_light;
}

// light_dir is from light to surface
float calculate_light_strength(vec3 light_dir, vec3 normal, vec3 view_dir) {
    float diffuse = clamp(dot(normal, -light_dir), 0.00, 1.0);
    diffuse *= 0.9;

    vec3 reflect_dir = reflect(-light_dir, normal);
    float specular   = pow(max(dot(view_dir, reflect_dir), 0.0), 3);
    // specular *= 0.0;

    return diffuse + specular;
}

layout(location = 0) in vec2 f_uv;

layout(location = 0) out vec4 o_color;

void main() {
    vec3 albedo = texture(textures[0], f_uv).xyz;
    vec3 normal = texture(textures[1], f_uv).xyz;
    float depth = texture(textures[2], f_uv).x;

    vec4 world4 = view.inv_proj_view * vec4(f_uv * 2.0 - 1.0, depth, 1.0);
    vec3 world  = world4.xyz / world4.w;

    vec3 view_pos = vec3(view.view_world_pos.xyz);
    vec3 view_dir = normalize(world - view_pos);

    vec3 light = calculate_total_light(world4, world, normal, view_dir);
    if (normal == vec3(0)) {
        light = vec3(1);
    }

    // o_color = vec4(f_uv,0,0);
    o_color = vec4(albedo * light, 1.0);
    // o_color = vec4(float(lights.point_light_count).xxx / 15,1.0);
    // o_color = vec4(vec3(length(world - view_pos)),1.0) / 100.f;
    // o_color = vec4(world * 0.01,1.0);
    // o_color = texture(textures[1] ,f_uv) * 0.5 + 0.5;
    // o_color = vec4(pow(texture(textures[2],f_uv).x,1).xxx,1.0);
    // o_color = mix(o_color, debug_color, 1.0);
    // o_color = vec4(albedo,1.0);
}