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
    mat4 lightSpaceMatrix;
    vec4 baseColorFactor;

    // x = nodeTransformsStartIdx
    // y = modelTransformIdx
    // z = materialIndex
    // w = pointLightCount
    uvec4 indices;

    // x = alphaCutoff
    // yzw = unused
    vec4 materialParams;

    vec4 directionalLightColor;

    // xyz = direction, w = intensity
    vec4 directionalLightDirectionIntensity;

    // xyz = camera position, w = unused
    vec4 cameraPos;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 outWorldSpaceNormal;
layout(location = 2) out vec3 outWorldSpaceVertex;
layout(location = 3) out vec3 outCameraForward;
layout(location = 4) out vec4 outFragPosLightSpace;

void main() 
{    
    mat4 model = ssbo.transforms[pc.indices.y];
    mat4 node  = ssbo.transforms[pc.indices.x + gl_InstanceIndex];
    mat4 world = model * node;

    vec4 worldPos = world * vec4(inPosition, 1.0);

    gl_Position = ubo.proj * ubo.view * worldPos;

    fragTexCoord = inTexCoord;
    outWorldSpaceVertex = worldPos.xyz;
    outCameraForward = pc.cameraPos.xyz - worldPos.xyz;
    outFragPosLightSpace = pc.lightSpaceMatrix * vec4(model * vec4(inPosition, 1.0));

    mat4 NormalMatrix = ssbo.transforms[pc.indices.y + 1];

    mat3 fullNormalMatrix = mat3(NormalMatrix) * mat3(ssbo.transforms[pc.indices.x + gl_InstanceIndex]);
    outWorldSpaceNormal = normalize(mat3(fullNormalMatrix ) * inNormal);
}