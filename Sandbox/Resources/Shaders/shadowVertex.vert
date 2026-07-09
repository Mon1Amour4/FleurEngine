#version 450

#extension GL_KHR_vulkan_glsl : enable

#define TRANSFORMS_MAX_CUP 1023

layout(push_constant) uniform PushConsts
{
    mat4 lightSpaceMatrix;
    uint modelIdx;
    uint nodeIdx;
} pc;

layout(std430, set = 0, binding = 0) readonly buffer SsboBuf
{
    mat4 transforms[TRANSFORMS_MAX_CUP];
} ssbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

void main()
{
    vec4 worldPos = ssbo.transforms[pc.modelIdx] * ssbo.transforms[pc.nodeIdx + gl_InstanceIndex] * vec4(inPosition, 1);
    gl_Position = pc.lightSpaceMatrix * worldPos;
}
