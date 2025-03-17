#version 450

#include "scene_set.glsl"
#include "material_set.glsl"

layout(location = 0) in vec3 f_color;
layout(location = 1) in vec2 f_texture_coord;
layout(location = 2) in vec3 f_normal;
layout(location = 3) in vec3 f_position;

layout(location = 0) out vec4 o_color;



#define LIGHT
#ifdef LIGHT
#define LIGHT_SET 2

// layout(set = LIGHT_SET, binding = 0, std430) readonly buffer Lights {
//     SceneLightData lights;
// };

// struct Tile {
//     uint light_count;
//     uint light_indicies[MAX_LIGHTS];
// };

// layout(set = LIGHT_SET, binding = 0) readonly buffer Tiles {
//     Tile tiles[];
// };

float calculate_light_strength(vec3 light_dir, vec3 normal, vec3 view_dir);

vec3 calculate_total_light(vec3 world_pos, vec3 normal, vec3 view_dir) {
    vec3 total_light = vec3(0);

    //calculate point lights
    for (int i = 0; i < min(MAX_LIGHTS, lights.point_light_count); i++) {
        PointLight p   = lights.point_lights[i];
        vec3 light_dir = world_pos - p.pos;
        float d        = length(light_dir);
        light_dir /= d;
        //if light is in range calculate it and add it to total
        if (d < p.range) {
            float strength = calculate_light_strength(light_dir, normal, view_dir);
            float mul      = 1.0 - (d / p.range);
            strength *= mul * mul;
            total_light += p.color.xyz * strength;

            // total_light += normalize(light_dir);
        }

        
    }

    //calculate directional light
    total_light += calculate_light_strength(lights.directional_light.dir.xyz, normal, view_dir) * lights.directional_light.color.xyz;
    
    //add ambient light
    total_light += lights.ambient_light.xyz;

    return total_light;
}

#else

#endif

// light_dir is from light to surface
float calculate_light_strength(vec3 light_dir, vec3 normal, vec3 view_dir) {
    float diffuse  = clamp(dot(normal, -light_dir), 0.05, 1.0);
    diffuse *= 0.9;

    vec3 reflect_dir = reflect(-light_dir, normal);
    float specular   = pow(max(dot(view_dir, reflect_dir), 0.0), 3);
    // specular *= 0.0;

    return diffuse + specular;
}

void main() {
    vec3 view_pos = vec3(scene_view.view_world_pos.xyz);
    vec3 view_dir = normalize(f_position - view_pos);

    vec3 albedo = texture(textures[0], f_texture_coord).xyz;

    o_color = vec4(albedo, 1);
    // o_color = vec4(1);

    vec3 light = calculate_total_light(f_position, f_normal, view_dir);

    o_color = vec4(albedo * light, 1.0);

    // o_color = vec4(f_normal * 0.5 + 0.5, 1) * vec4(0,1,0,1);
    // o_color.y = step(0.6,o_color.y);
    // o_color = vec4(f_normal * 0.5 + 0.5, 1);
    // o_color = vec4(f_normal , 1);
    // o_color = vec4(light ,1.0);

    // o_color = vec4(vec3(dot(f_normal,-light)),1.0);

    // o_color = vec4(view_dir * 0.5 + 0.5,1.0);
}