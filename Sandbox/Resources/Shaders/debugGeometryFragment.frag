#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler[];

layout(push_constant) uniform PushConsts
{
    mat4 viewProj;
    int textureIdx;
    vec4 color;
} pc;

void main()
{
    if (pc.textureIdx >= 0)
    {
        outColor = texture(texSampler[pc.textureIdx], uv);
    } else
    {
        outColor = pc.color;
    }
}
