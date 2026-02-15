#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler[];

layout(push_constant) uniform PushConsts
{
    uint materialIndex;
} pc;


void main() {
    outColor = texture(texSampler[pc.materialIndex], fragTexCoord);
}