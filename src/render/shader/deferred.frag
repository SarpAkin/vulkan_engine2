#version 450

#define DEFERRED_SET 0

#include "scene_data.h"

layout(set = DEFERRED_SET,binding = 0) uniform sampler2D textures[3];

layout(set = DEFERRED_SET,binding = 1) uniform ViewDataBuffer{
    ViewData view;
};

layout(set = DEFERRED_SET,binding = 2) readonly buffer LightsBuffer{
    SceneLightData lights;
    PointLight point_lights[];
};

float calculate_light_strength(vec3 light_dir, vec3 normal, vec3 view_dir);

vec3 calculate_total_light(vec3 world_pos, vec3 normal, vec3 view_dir) {
    vec3 total_light = vec3(0);

    //calculate point lights
    for (int i = 0; i < min(MAX_LIGHTS, lights.point_light_count); i++) {
        PointLight p   = point_lights[i];
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

// light_dir is from light to surface
float calculate_light_strength(vec3 light_dir, vec3 normal, vec3 view_dir) {
    float diffuse  = clamp(dot(normal, -light_dir), 0.05, 1.0);
    diffuse *= 0.9;

    vec3 reflect_dir = reflect(-light_dir, normal);
    float specular   = pow(max(dot(view_dir, reflect_dir), 0.0), 3);
    // specular *= 0.0;

    return diffuse + specular;
}


layout (location = 0) in vec2 f_uv;

layout (location = 0) out vec4 o_color;


void main(){
    vec3 albedo = texture(textures[0],f_uv).xyz; 
    vec3 normal = texture(textures[1],f_uv).xyz;
    float depth = texture(textures[2],f_uv).x;

    vec4 world4 = view.inv_proj_view * vec4(f_uv * 2.0 - 1.0,depth,1.0);
    vec3 world = world4.xyz / world4.w;

    vec3 view_pos = vec3(view.view_world_pos.xyz);
    vec3 view_dir = normalize(world - view_pos);


    vec3 light = calculate_total_light(world,normal,view_dir);

    // o_color = vec4(f_uv,0,0);
    o_color = vec4(albedo  * light,1.0);
    // o_color = vec4(float(lights.point_light_count).xxx / 15,1.0);
    // o_color = vec4(vec3(length(world - view_pos)),1.0) / 100.f;

    // o_color = texture(textures[1] ,f_uv) * 0.5 + 0.5;
    // o_color = vec4(pow(texture(textures[2],f_uv).x,1).xxx,1.0);

}