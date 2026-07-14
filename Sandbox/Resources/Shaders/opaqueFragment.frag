#version 450

#extension GL_EXT_nonuniform_qualifier : enable

#define POINT_LIGHTS_MAX_CUP 32

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 worldSpaceNormal;
layout(location = 2) in vec3 worldSpaceVertex;
layout(location = 3) in vec3 cameraForward;
layout(location = 4) in vec4 FragPosLightSpace;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler[];

struct PointLight
{
  vec3 pos;
  float radius;

  vec3 color;
  float intensity;
};

layout(std430, set = 3, binding = 0) readonly buffer PointLightBuff
{
    PointLight lights[POINT_LIGHTS_MAX_CUP];
} pointLights;

layout(set = 4, binding = 0) uniform sampler2D shadowMapSampler;

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

float ShadowCalculation(vec4 fragPosLightSpace, float NdotL)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // glm::orthoRH_ZO already produces Vulkan depth in [0, 1].
    // Only x/y are in the conventional [-1, 1] NDC range and need remapping.
    // Without this guard, UVs wrap on the shared sampler and can zero out
    // the directional term across the whole scene.
    if (projCoords.z < 0.0 || projCoords.z > 1.0)
        return 0.0;

    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMapSampler, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    float bias = max(0.0005, 0.005 * (1.0 - NdotL));
    // check whether current frag pos is in shadow
    // 1 - in shadow
    // 0 - isn't in sahdow 
    float shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;

    return shadow;
}

void main() 
{
    vec3 V = normalize(cameraForward);
    // Directional light is stored as the ray direction, so shading needs the opposite vector.
    vec3 L = normalize(-pc.directionalLightDirectionIntensity.xyz);
    float I = pc.directionalLightDirectionIntensity.w;
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
    vec4 diffuse  = albedo * NdotL * vec4(pc.directionalLightColor.xyz, 1.0);
    vec4 specular = vec4(1.0) * pow(NdotH, shininess);

    vec4 pointLightColor = vec4(0,0,0,1);
    for (int i = 0; i < pc.indices.w; i++)
    {
        //vec3 lightDir = worldSpaceNormal - (pointLights.lights[i].pos);
        vec3 lightDir = pointLights.lights[i].pos - worldSpaceVertex;
        float lightLength = length(lightDir);

        if (lightLength < pointLights.lights[i].radius)
        {
            float lightDotN = max(0, dot(worldSpaceNormal, normalize(lightDir)));
            pointLightColor = pointLightColor +  vec4(pointLights.lights[i].color,1) * pointLights.lights[i].intensity * lightDotN;
        }
    }
    float shadow = ShadowCalculation(FragPosLightSpace, NdotL);  
    // TODO: split lighting terms explicitly. Right now only the directional diffuse/specular
    // term is shadowed; decide whether point lights should stay unshadowed or get their own shadows.
    outColor = ambient + pointLightColor + ((diffuse + specular) * I) * (1.0 - shadow);
    outColor.a = albedo.a;
    //outColor = ambient + pointLightColor + ((diffuse + specular) * I);
    //outColor = ambient + pointLightColor + (diffuse) * I;
}


