#version 450
#extension GL_KHR_vulkan_glsl : enable

// IN
layout(location = 0) in vec2 TexCoords;
layout(location = 1) in vec3 wNormal;
layout(location = 2) in vec3 wFragPos;
layout(location = 3) in vec3 cameraForward;
layout(location = 4) in vec4 wTangent;

// OUT
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

layout(push_constant) uniform DrawPushConstants
{
    vec4 baseColorFactor;
    uvec4 drawIndices;
    vec4 materialParams;
    uvec4 textureIndices;
} draw;

layout(set = 0, binding = 0) uniform SceneData
{
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 directionalLightColor;
    vec4 directionalLightDirectionAndIntensity;
} scene;

// RESOURCES
layout(set = 1, binding = 0) uniform sampler2D diffuse;
layout(set = 1, binding = 1) uniform sampler2D specular;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = vec4(wFragPos, 1.0);
    // also store the per-fragment normals into the gbuffer
    gNormal = vec4(normalize(wNormal), 1.0);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(diffuse, TexCoords).rgb * draw.baseColorFactor.rgb;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(specular, TexCoords).r * draw.baseColorFactor.a + scene.view[0][0] * 0.0;
} 
