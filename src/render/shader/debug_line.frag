#version 450 core

layout(location = 0) in vec3 f_color;

layout(location = 0) out vec4 o_color;
#ifdef GPASS
layout(location = 1) out vec4 o_normal;
#endif

void main() {
    o_color = vec4(f_color, 1.0);

#ifdef GPASS
    o_normal = vec4(0.0);
#endif
}