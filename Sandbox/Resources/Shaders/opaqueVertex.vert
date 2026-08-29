#version 450

#define TRANSFORMS_MAX_CUP 1023

#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 directionalLightColor;
    vec4 directionalLightDirectionAndIntensity;
} scene;

layout(std430, set = 2, binding = 0) readonly buffer TransformBuffer
{
    mat4 transforms[TRANSFORMS_MAX_CUP];
} transformBuffer;

layout(push_constant) uniform DrawPushConstants
{
    vec4 baseColorFactor;

    // x = node transform start index
    // y = model transform index
    // z = unused
    // w = point light count
    uvec4 drawIndices;

    // x = alpha cutoff
    // y = directional shadow sampling mode
    // z = point light shadow sampling mode
    // w = normal mapping enabled (temporary F4 debug toggle)
    vec4 materialParams;

    // x = albedo texture index
    // y = normal map texture index
    // zw = unused
    uvec4 textureIndices;
} draw;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 outWorldSpaceNormal;
layout(location = 2) out vec3 outWorldSpaceVertex;
layout(location = 3) out vec3 outCameraForward;
layout(location = 4) out vec4 outWorldSpaceTangent;

void main() 
{    
    mat4 model = transformBuffer.transforms[draw.drawIndices.y];
    mat4 node  = transformBuffer.transforms[draw.drawIndices.x + gl_InstanceIndex];
    mat4 world = model * node;

    vec4 worldPos = world * vec4(inPosition, 1.0);

    gl_Position = scene.proj * scene.view * worldPos;

    fragTexCoord = inTexCoord;
    outWorldSpaceVertex = worldPos.xyz;
    outCameraForward = scene.cameraPos.xyz - worldPos.xyz;

    mat3 worldLinear = mat3(world);
    mat3 worldNormal = transpose(inverse(worldLinear));
    outWorldSpaceNormal = normalize(worldNormal * inNormal);

    // Tangents are directions, so they use the ordinary linear transform.
    // A mirrored transform reverses the TBN orientation and must flip handedness.
    float determinantSign = determinant(worldLinear) < 0.0 ? -1.0 : 1.0;
    outWorldSpaceTangent = vec4(worldLinear * inTangent.xyz, inTangent.w * determinantSign);
}
