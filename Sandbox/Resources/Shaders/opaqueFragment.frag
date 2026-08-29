#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

struct PointLight
{
  vec3 pos;
  float radius;

  vec3 color;
  float intensity;
};

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

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 worldSpaceNormal;
layout(location = 2) in vec3 worldSpaceVertex;
layout(location = 3) in vec3 cameraForward;
layout(location = 4) in vec4 worldSpaceTangent;
layout(set = 0, binding = 0) uniform SceneData
{
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 directionalLightColor;
    vec4 directionalLightDirectionAndIntensity;
} scene;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler[4096];
layout(set = 4, binding = 0) uniform sampler2DArray shadowMapSampler;
layout(std140, set = 4, binding = 1) uniform DirectionalShadowMatrices
{
    mat4 lightSpaceMatrices[16];
    uint cascadeCount;
    vec4 cascadeSplits[4];
} shadow;
layout(set = 5, binding = 0) uniform sampler3D shadowMapOffsetTexture;
layout(std430, set = 3, binding = 0) readonly buffer PointLightBuff
{
    PointLight lights[100];
} pointLights;

layout(set = 6, binding = 0) uniform samplerCube pointLightShadowMaps[100];

const uint LIGHT_SAMPLING_DEFAULT = 0u;
const uint LIGHT_SAMPLING_PCF_3X3 = 1u;
const uint LIGHT_SAMPLING_PCF_5X5 = 2u;
const uint LIGHT_SAMPLING_NOISE_TEXTURE = 3u;
const float pointLightShadowNear = 0.1;
const float pointLightShadowBias = 0.05;

#include "shadowFunctions.glsl"

void main() 
{
    // Reconstruct camera depth per fragment to avoid interpolation errors at
    // cascade boundaries.
    float fragmentCameraDepth = -(scene.view * vec4(worldSpaceVertex, 1.0)).z;

    vec3 V = normalize(cameraForward);
    // Directional light is stored as the ray direction, so shading needs the opposite vector.
    vec3 L = normalize(-scene.directionalLightDirectionAndIntensity.xyz);
    float directionalIntensity = scene.directionalLightDirectionAndIntensity.w;
    // TEMP_DEBUG_NORMAL_MAP_SPECULAR: wider highlight for normal-map inspection.
    float shininess = 64;

    vec4 albedo = texture(texSampler[draw.textureIndices.x], fragTexCoord) * draw.baseColorFactor;
    if (albedo.a < draw.materialParams.x)
    {
        discard;
    }

    vec3 geometricNormal = normalize(worldSpaceNormal);
    vec3 normal = geometricNormal;

    // textureIndices.y is UINT_MAX when the material has no normal texture.
    // Keep the sentinel out of the nonuniform sampler array indexing operation.
    if (draw.materialParams.w > 0.5 && draw.textureIndices.y != 0xffffffffu)
    {
        vec3 tangentNormal = texture(texSampler[draw.textureIndices.y], fragTexCoord).rgb;
        tangentNormal = normalize(tangentNormal * 2.0 - 1.0);

        vec3 N = geometricNormal;
        vec3 T = normalize(worldSpaceTangent.xyz);
        T = normalize(T - N * dot(N, T));
        vec3 B = normalize(cross(N, T)) * worldSpaceTangent.w;
        normal = normalize(mat3(T, B, N) * tangentNormal);
    }

    float NdotL = dot(normal, L);
    float NdotH = 0.0;

    if (NdotL > 0.0)
    {
        vec3 H = normalize(L + V);
        NdotH = max(0.0, dot(normal, H));
    }

    NdotL = max(0.0, NdotL);

    vec4 ambient  = albedo * 0.05;
    vec4 directionalDiffuse  = albedo * NdotL * vec4(scene.directionalLightColor.xyz, 1.0);
    vec4 directionalSpecular = vec4(1.0) * pow(NdotH, shininess);

    vec3 pointLightsColor = vec3(0.0);
    for (int i = 0; i < draw.drawIndices.w; i++)
    {
        vec3 lightVector = pointLights.lights[i].pos - worldSpaceVertex;
        float lightDistance = length(lightVector);

        if (lightDistance <= pointLights.lights[i].radius)
        {
            vec3 pointLightDirection = normalize(lightVector);
            float pointLightNdotL = max(0.0, dot(normal, pointLightDirection));
            float attenuation = 1.0 - lightDistance / pointLights.lights[i].radius;

            vec3 pointDiffuse = albedo.rgb * pointLightNdotL;

            float pointLightNdotH = 0.0;
            if (pointLightNdotL > 0.0)
            {
                vec3 pointH = normalize(pointLightDirection + V);
                pointLightNdotH = max(0.0, dot(normal, pointH));
            }

            vec3 pointSpecular = vec3(pow(pointLightNdotH, shininess));
            float pointLightVisibility = 1.0 - PointLightShadowCalculation(i, worldSpaceVertex);

            pointLightsColor += (pointDiffuse + pointSpecular) *
                                pointLights.lights[i].color *
                                pointLights.lights[i].intensity *
                                attenuation * pointLightVisibility;
        }
    }
    float directionalVisibility = 1 - ShadowCalculation(worldSpaceVertex, fragmentCameraDepth, NdotL);
    // TODO: split lighting terms explicitly. Right now only the directional diffuse/specular
    // term is shadowed; decide whether point lights should stay unshadowed or get their own shadows.
    vec4 directionalLightContribution = (directionalDiffuse + directionalSpecular) * directionalIntensity * directionalVisibility;


    outColor = ambient + directionalLightContribution + vec4(pointLightsColor, 0.0);
    outColor.a = albedo.a;
}


