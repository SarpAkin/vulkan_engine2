#version 450 core

layout(location = 0) in vec3 v_color;

layout (location = 0) out vec3 f_color;

layout(set = 0,binding = 0) uniform sampler2D image;

void main(){

    f_color = v_color;
    f_color = texture(image,v_color.xy).xyz;
}
