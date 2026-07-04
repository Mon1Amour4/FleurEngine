#version 450

#define TRANSFORMS_MAX_CUP 1023

layout(push_constant) uniform PushConsts
{
    mat4 viewProj;
} pc;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 vColor;

void main()
{
    gl_Position = pc.viewProj * vec4(inPosition, 1.0);
    gl_PointSize = 8.0;  // point topology only; ignored for lines
    vColor = inColor;
}
