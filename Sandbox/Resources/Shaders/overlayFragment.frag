#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler[];

layout(push_constant) uniform PushConsts
{
    ivec4 params;
    vec4 color;
} pc;

void main()
{
    if (pc.params.y == 1 && pc.params.x >= 0)
    {
        outColor = texture(texSampler[pc.params.x], uv);
    }
    else if (pc.params.y == 2 && pc.params.x >= 0)
    {
        float depth = texture(texSampler[pc.params.x], uv).r;
        outColor = vec4(depth, depth, depth, 1.0);
    }
    else
    {
        outColor = pc.color;
    }
}
