#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 finalColor;

layout(set = 0, binding = 0) uniform SceneData
{
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 directionalLightColor;
    vec4 directionalLightDirectionAndIntensity;
} scene;

layout(set = 1, binding = 0) uniform sampler2D gPosition;
layout(set = 1, binding = 1) uniform sampler2D gNormal;
layout(set = 1, binding = 2) uniform sampler2D gAlbedoSpec;

const int maxPointLights = 100;
const int shadowedPointLights = maxPointLights;

struct PointLight { vec3 pos; float radius; vec3 color; float intensity; };
layout(std430, set = 2, binding = 0) readonly buffer PointLightBuff { PointLight lights[maxPointLights]; } pointLights;
layout(set = 3, binding = 0) uniform sampler2DArray shadowMapSampler;
layout(std140, set = 3, binding = 1) uniform DirectionalShadowMatrices { mat4 lightSpaceMatrices[16]; uint cascadeCount; vec4 cascadeSplits[4]; } shadow;
layout(set = 4, binding = 0) uniform samplerCube pointLightShadowMaps[maxPointLights];

void main()
{
    vec3 fragPos = texture(gPosition, texCoords).xyz;
    vec3 normal = normalize(texture(gNormal, texCoords).xyz);
    vec4 albedoSpec = texture(gAlbedoSpec, texCoords);
    vec3 viewDirection = normalize(scene.cameraPos.xyz - fragPos);
    vec3 lightDirection = normalize(-scene.directionalLightDirectionAndIntensity.xyz);
    float nDotL = max(dot(normal, lightDirection), 0.0);
    vec3 radiance = scene.directionalLightColor.rgb * scene.directionalLightDirectionAndIntensity.w;
    vec3 halfDirection = normalize(lightDirection + viewDirection);
    float specular = nDotL > 0.0 ? pow(max(dot(normal, halfDirection), 0.0), 200.0) * albedoSpec.a : 0.0;
    float cameraDepth = -(scene.view * vec4(fragPos, 1.0)).z;
    int cascade = 0;
    for (int i = 0; i < int(shadow.cascadeCount); ++i) { if (cameraDepth <= shadow.cascadeSplits[i / 4][i % 4]) { cascade = i; break; } cascade = i; }
    vec4 lightSpace = shadow.lightSpaceMatrices[cascade] * vec4(fragPos, 1.0);
    vec3 shadowCoord = lightSpace.xyz / lightSpace.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
    float directionalShadow = (shadowCoord.z >= 0.0 && shadowCoord.z <= 1.0 && all(greaterThanEqual(shadowCoord.xy, vec2(0.0))) && all(lessThanEqual(shadowCoord.xy, vec2(1.0))))
        ? float(texture(shadowMapSampler, vec3(shadowCoord.xy, cascade)).r + max(0.0005, 0.005 * (1.0 - nDotL)) < shadowCoord.z) : 0.0;
    vec3 color = albedoSpec.rgb * 0.05 + (albedoSpec.rgb * nDotL + vec3(specular)) * radiance * (1.0 - directionalShadow);
    for (int i = 0; i < maxPointLights; ++i)
    {
        const PointLight pointLight = pointLights.lights[i];
        const vec3 toLight = pointLight.pos - fragPos;
        const float distanceToLight = length(toLight);
        if (distanceToLight > pointLight.radius)
            continue;

        const vec3 lightDirection = toLight / distanceToLight;
        const float pointNdotL = max(dot(normal, lightDirection), 0.0);
        const vec3 halfVector = normalize(lightDirection + viewDirection);
        const float pointSpecular = pointNdotL > 0.0 ? pow(max(dot(normal, halfVector), 0.0), 200.0) * albedoSpec.a : 0.0;

        float visibility = 1.0;
        if (i < shadowedPointLights)
        {
            const vec3 shadowDirection = normalize(fragPos - pointLight.pos);
            const float storedDepth = texture(pointLightShadowMaps[nonuniformEXT(i)], shadowDirection).r;
            const float nearPlane = 0.1;
            const float farPlane = pointLight.radius;
            const float closestDepth = (nearPlane * farPlane) / (farPlane - storedDepth * (farPlane - nearPlane));
            const float currentDepth = max(max(abs(fragPos.x - pointLight.pos.x), abs(fragPos.y - pointLight.pos.y)), abs(fragPos.z - pointLight.pos.z));
            visibility = float(currentDepth - 0.05 <= closestDepth);
        }

        const float attenuation = 1.0 - distanceToLight / pointLight.radius;
        color += (albedoSpec.rgb * pointNdotL + vec3(pointSpecular)) * pointLight.color * pointLight.intensity * attenuation * visibility;
    }
    finalColor = vec4(color, 1.0);
}
