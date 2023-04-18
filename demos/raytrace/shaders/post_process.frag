#version 450 core

layout(location = 0) in vec2 v_uv;

layout(location = 0 ) out vec4 albedo;

layout(set = 0,binding = 0) uniform sampler2D voxel_results;

void main(){
    albedo = texture(voxel_results,v_uv);
}