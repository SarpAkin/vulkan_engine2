#version 450

#define DEFERRED_SET 0

layout(set = DEFERRED_SET,binding = 0) uniform sampler2D textures[3];


layout (location = 0) in vec2 f_uv;

layout (location = 0) out vec4 o_color;


void main(){
    o_color = vec4(f_uv,0,0);
    o_color = texture(textures[0],f_uv);
    o_color = texture(textures[1] ,f_uv) * 0.5 + 0.5;
    // o_color = vec4(pow(texture(textures[2],f_uv).x,1).xxx,1.0);

}