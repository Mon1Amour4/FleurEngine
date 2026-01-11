#version 450

#define MAX_TEXTURES 128

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler[MAX_TEXTURES];

layout(push_constant) uniform PushConsts
{
    uint materialIndex;
} pc;


void main() {
    outColor = texture(texSampler[pc.materialIndex], fragTexCoord);
}