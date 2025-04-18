#version 450

//0-1 range
layout (location = 0) out vec2 f_uv;

void main() 
{
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * 2.0f + -1.0f, 0.0f, 1.0f);
    f_uv = uv;
}