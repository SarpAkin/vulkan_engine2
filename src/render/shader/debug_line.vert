#version 450 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in uint v_color;

layout(location = 0) out vec3 f_color;

layout(push_constant) uniform Push{
    mat4 proj_view;
};

void main(){
    gl_Position = proj_view * vec4(v_position,1.0);
    f_color = unpackUnorm4x8(v_color).xyz;
}