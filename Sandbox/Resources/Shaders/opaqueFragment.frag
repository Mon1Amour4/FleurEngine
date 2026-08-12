#version 450

#extension GL_EXT_nonuniform_qualifier : enable

struct PointLight
{
  vec3 pos;
  float radius;

  vec3 color;
  float intensity;
};

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
    // y = directional LightSampling
    // z = point LightSampling
    // w = unused
    vec4 materialParams;

    vec4 directionalLightColor;

    // xyz = direction, w = intensity
    vec4 directionalLightDirectionIntensity;

    // xyz = camera position, w = unused
    vec4 cameraPos;
} pc;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 worldSpaceNormal;
layout(location = 2) in vec3 worldSpaceVertex;
layout(location = 3) in vec3 cameraForward;
layout(set = 0, binding = 0) uniform CameraMatrices
{
    mat4 view;
    mat4 proj;
} camera;

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
    PointLight lights[12];
} pointLights;

layout(set = 6, binding = 0) uniform samplerCube pointLightShadowMaps[12];

const uint LIGHT_SAMPLING_DEFAULT = 0u;
const uint LIGHT_SAMPLING_PCF_3X3 = 1u;
const uint LIGHT_SAMPLING_PCF_5X5 = 2u;
const uint LIGHT_SAMPLING_NOISE_TEXTURE = 3u;
const float pointLightShadowNear = 0.1;
const float pointLightShadowBias = 0.05;

float PointLightShadowCalculation(int lightIndex, vec3 fragPos)
{
    float shadowFar = pointLights.lights[lightIndex].radius;

    vec3 fragToLight = fragPos - pointLights.lights[lightIndex].pos;
    // Perspective depth stores distance along the selected cubemap face axis,
    // not the radial distance to the point light.
    float currentDistance = max(abs(fragToLight.x),
                                max(abs(fragToLight.y), abs(fragToLight.z)));

    vec3 direction = normalize(fragToLight);
    const uint samplingMode = uint(pc.materialParams.z + 0.5);
    if (samplingMode == LIGHT_SAMPLING_DEFAULT)
    {
        float shadowMapDepth = texture(pointLightShadowMaps[nonuniformEXT(lightIndex)], direction).r;
        float closestDistance = (pointLightShadowNear * shadowFar) /
                                (shadowFar - shadowMapDepth * (shadowFar - pointLightShadowNear));
        return float(currentDistance - pointLightShadowBias > closestDistance);
    }

    const int filterSize = samplingMode == LIGHT_SAMPLING_PCF_5X5 ? 5 : 3;
    const int sampleCount = filterSize * filterSize;
    vec3 tangent = normalize(cross(direction, abs(direction.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));
    vec3 bitangent = normalize(cross(direction, tangent));
    vec2 noiseUv = (floor(gl_FragCoord.xy) + 0.5) / vec2(textureSize(shadowMapOffsetTexture, 0).xy);
    float shadow = 0.0;
    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        ivec2 kernel = ivec2(sampleIndex % filterSize, sampleIndex / filterSize) - ivec2(filterSize / 2);
        vec2 offset = vec2(kernel);
        if (samplingMode == LIGHT_SAMPLING_NOISE_TEXTURE)
        {
            float z = (float(sampleIndex) + 0.5) / float(sampleCount);
            offset = texture(shadowMapOffsetTexture, vec3(noiseUv, z)).rg;
        }
        vec3 sampleDirection = normalize(direction + (tangent * offset.x + bitangent * offset.y) * 0.003);
        float shadowMapDepth = texture(pointLightShadowMaps[nonuniformEXT(lightIndex)], sampleDirection).r;
        float closestDistance = (pointLightShadowNear * shadowFar) /
                                (shadowFar - shadowMapDepth * (shadowFar - pointLightShadowNear));
        shadow += float(currentDistance - pointLightShadowBias > closestDistance);
    }
    return shadow / float(sampleCount);
}

float GetCascadeSplit(int index)
{
    return shadow.cascadeSplits[index / 4][index % 4];
}

int SelectCascade(float cameraDepth)
{
    for (int i = 0; i < int(shadow.cascadeCount); ++i)
    {
        if (cameraDepth <= GetCascadeSplit(i))
            return i;
    }
    return max(0, int(shadow.cascadeCount) - 1);
}

float ShadowCalculationForCascade(vec3 worldPosition, int cascadeIndex, float NdotL)
{
    vec4 fragPosLightSpace = shadow.lightSpaceMatrices[cascadeIndex] * vec4(worldPosition, 1.0);

    if (abs(fragPosLightSpace.w) < 0.000001)
        return 0.0;

    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Fleur::Math::orthoRH_ZO already produces Vulkan depth in [0, 1].
    // Only x/y are in the conventional [-1, 1] NDC range and need remapping.
    // Without this guard, UVs wrap on the shared sampler and can zero out
    // the directional term across the whole scene.
    if (projCoords.z < 0.0 || projCoords.z > 1.0)
        return 0.0;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    float bias = max(0.0005, 0.005 * (1.0 - NdotL));

    const uint samplingMode = uint(pc.materialParams.y + 0.5);
    float shadow = 0.0;

    if (samplingMode == LIGHT_SAMPLING_DEFAULT)
    {
        float shadowMapDepth = texture(shadowMapSampler, vec3(projCoords.xy, float(cascadeIndex))).x;
        shadow = float(shadowMapDepth + bias < currentDepth);
    }
    else
    {
        const int filterSize = samplingMode == LIGHT_SAMPLING_PCF_5X5 ? 5 : 3;
        const int sampleCount = filterSize * filterSize;
        vec2 texelSize = 1.0 / vec2(textureSize(shadowMapSampler, 0));
        vec2 offsetTextureCoord = (floor(gl_FragCoord.xy) + 0.5) /
                                  vec2(textureSize(shadowMapOffsetTexture, 0).xy);

        for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
        {
            ivec2 kernelOffset = ivec2(sampleIndex % filterSize, sampleIndex / filterSize) - ivec2(1);
            vec2 offset = vec2(kernelOffset);
            if (samplingMode == LIGHT_SAMPLING_NOISE_TEXTURE)
            {
                float z = (float(sampleIndex) + 0.5) / float(sampleCount);
                offset = texture(shadowMapOffsetTexture, vec3(offsetTextureCoord, z)).rg;
            }

            float depth = texture(shadowMapSampler,
                                  vec3(projCoords.xy + offset * texelSize, float(cascadeIndex))).x;
            shadow += float(depth + bias < currentDepth);
        }

        shadow /= float(sampleCount);
    }

//    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
//    float closestDepth = texture(shadowMapSampler, vec3(projCoords.xy, 0.0)).r;

    //float bias = 0;
    // check whether current frag pos is in shadow
    // 1 - in shadow
    // 0 - isn't in sahdow 
    //float shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;

    return shadow;
}

float ShadowCalculation(vec3 worldPosition, float cameraDepth, float NdotL)
{
    if (shadow.cascadeCount == 0u)
        return 0.0;

    const int cascadeIndex = SelectCascade(cameraDepth);
    return ShadowCalculationForCascade(worldPosition, cascadeIndex, NdotL);
}

void main() 
{
    // Reconstruct camera depth per fragment to avoid interpolation errors at
    // cascade boundaries.
    float fragmentCameraDepth = -(camera.view * vec4(worldSpaceVertex, 1.0)).z;

    vec3 V = normalize(cameraForward);
    // Directional light is stored as the ray direction, so shading needs the opposite vector.
    vec3 L = normalize(-pc.directionalLightDirectionIntensity.xyz);
    float DirectionalIntensity = pc.directionalLightDirectionIntensity.w;
    float shininess = 200;

    vec4 albedo = texture(texSampler[pc.indices.z], fragTexCoord) * pc.baseColorFactor;
    if (albedo.a < pc.materialParams.x) 
    {
        discard;
    }

    float NdotL = dot(worldSpaceNormal, L);
    float NdotH = 0.0;

    if (NdotL > 0.0)
    {
        vec3 H = normalize(L + V);
        NdotH = max(0.0, dot(worldSpaceNormal, H));
    }

    NdotL = max(0.0, NdotL);

    vec4 ambient  = albedo * 0.05;
    vec4 DirectionalDiffuse  = albedo * NdotL * vec4(pc.directionalLightColor.xyz, 1.0);
    vec4 DirectionalSpecular = vec4(1.0) * pow(NdotH, shininess);

    vec3 PointLightsColor = vec3(0.0);
    for (int i = 0; i < pc.indices.w; i++)
    {
        vec3 lightVector = pointLights.lights[i].pos - worldSpaceVertex;
        float lightDistance = length(lightVector);

        if (lightDistance <= pointLights.lights[i].radius)
        {
            vec3 pointLightDirection = normalize(lightVector);
            float pointLightNdotL = max(0.0, dot(worldSpaceNormal, pointLightDirection));
            float attenuation = 1.0 - lightDistance / pointLights.lights[i].radius;

            vec3 pointDiffuse = albedo.rgb * pointLightNdotL;

            float pointLightNdotH = 0.0;
            if (pointLightNdotL > 0.0)
            {
                vec3 pointH = normalize(pointLightDirection + V);
                pointLightNdotH = max(0.0, dot(worldSpaceNormal, pointH));
            }

            vec3 pointSpecular = vec3(pow(pointLightNdotH, shininess));
            float pointLightVisibility = 1.0 - PointLightShadowCalculation(i, worldSpaceVertex);

            PointLightsColor += (pointDiffuse + pointSpecular) *
                                pointLights.lights[i].color *
                                pointLights.lights[i].intensity *
                                attenuation * pointLightVisibility;
        }
    }
    float DirectionalVisibility = 1 - ShadowCalculation(worldSpaceVertex, fragmentCameraDepth, NdotL);
    // TODO: split lighting terms explicitly. Right now only the directional diffuse/specular
    // term is shadowed; decide whether point lights should stay unshadowed or get their own shadows.
    vec4 DirectionalLightColor = (DirectionalDiffuse + DirectionalSpecular) * DirectionalIntensity * DirectionalVisibility;


    outColor = ambient + DirectionalLightColor + vec4(PointLightsColor, 0.0);
    outColor.a = albedo.a;
}


