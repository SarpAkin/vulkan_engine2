#version 450 core

layout(location = 0) in vec3 v_uv;
layout(location = 1) in vec3 v_color;

layout (location = 0) out vec3 f_color;

layout(set = 0,binding = 0) uniform sampler2D image;

void main(){

    f_color = texture(image,v_uv.xy).xyz * v_color;
}
