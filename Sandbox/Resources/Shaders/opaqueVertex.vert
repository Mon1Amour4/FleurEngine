#version 450

#define TRANSFORMS_MAX_CUP 1023

#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(std430, set = 2, binding = 0) readonly buffer SsboBuf
{
    mat4 transforms[TRANSFORMS_MAX_CUP];
} ssbo;

layout(push_constant) uniform PushConsts
{
    vec4 baseColorFactor;
    uint nodeTransformsStartIdx;
    uint modelTransformIdx;
    uint materialIndex;
    float alphaCutoff;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ssbo.transforms[pc.modelTransformIdx] * ssbo.transforms[pc.nodeTransformsStartIdx + gl_InstanceIndex] * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}