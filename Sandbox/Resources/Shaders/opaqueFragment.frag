#version 450

#extension GL_EXT_nonuniform_qualifier : enable

#define POINT_LIGHTS_MAX_CUP 32

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 worldSpaceNormal;
layout(location = 2) in vec3 worldSpaceVertex;
layout(location = 3) in vec3 cameraForward;

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

layout(push_constant) uniform PushConsts
{
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


void main() 
{
    vec3 V = normalize(cameraForward);
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

    outColor = ambient + pointLightColor + (diffuse + specular) * I;
    //outColor = ambient + pointLightColor + (diffuse) * I;
}
