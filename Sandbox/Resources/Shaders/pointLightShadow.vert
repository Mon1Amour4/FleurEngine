#version 450

#define TRANSFORMS_MAX_CUP 1023

layout(push_constant) uniform PushConsts
{
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
    vec4 worldPosition = ssbo.transforms[pc.modelIdx] * ssbo.transforms[pc.nodeIdx + gl_InstanceIndex] * vec4(inPosition, 1.0);
    gl_Position = worldPosition;
}
