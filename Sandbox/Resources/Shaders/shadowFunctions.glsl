float PointLightShadowCalculation(int lightIndex, vec3 fragPos)
{
    float shadowFar = pointLights.lights[lightIndex].radius;

    vec3 fragToLight = fragPos - pointLights.lights[lightIndex].pos;
    // Perspective depth stores distance along the selected cubemap face axis,
    // not the radial distance to the point light.
    float currentDistance = max(abs(fragToLight.x),
                                max(abs(fragToLight.y), abs(fragToLight.z)));

    vec3 direction = normalize(fragToLight);
    const uint samplingMode = uint(draw.materialParams.z + 0.5);
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

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Fleur::Math::orthoRH_ZO already produces Vulkan depth in [0, 1].
    if (projCoords.z < 0.0 || projCoords.z > 1.0)
        return 0.0;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.0005, 0.005 * (1.0 - NdotL));

    const uint samplingMode = uint(draw.materialParams.y + 0.5);
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

    return shadow;
}

float ShadowCalculation(vec3 worldPosition, float cameraDepth, float NdotL)
{
    if (shadow.cascadeCount == 0u)
        return 0.0;

    const int cascadeIndex = SelectCascade(cameraDepth);
    return ShadowCalculationForCascade(worldPosition, cascadeIndex, NdotL);
}
