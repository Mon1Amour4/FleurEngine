#version 460

layout(location = 0) in vec3 inTexCoord;
layout (location=0) out vec4 outColor;

layout(set = 0, binding = 1) uniform samplerCube cubeSampler;

void main()
{
    outColor = texture(cubeSampler, inTexCoord);
}