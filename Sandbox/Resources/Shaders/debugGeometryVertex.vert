#version 450


layout(push_constant) uniform PushConsts
{
    mat4 viewProj;
    int textureIdx;
    vec4 color;
} pc;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec2 outUv;

void main()
{
    gl_Position = pc.viewProj * vec4(inPosition, 1.0);
    outUv = inUv;
}
