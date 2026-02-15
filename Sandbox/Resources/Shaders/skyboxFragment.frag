#version 460

layout (location=0) out vec4 outColor;

layout(push_constant) uniform PushConsts
{
    vec3 Dir;
} pc;

layout (set = 0, binding = 0) uniform samplerCube cubeSampler;

void main()
{
    outColor = texture(cubeSampler, pc.Dir);
}