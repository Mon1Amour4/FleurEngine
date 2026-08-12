#version 450

#define MAX_CASCADE_COUNT 16

layout(triangles, invocations = MAX_CASCADE_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(std140, set = 1, binding = 0) uniform DirectionalShadowMatrices
{
    mat4 lightSpaceMatrices[MAX_CASCADE_COUNT];
    uint cascadeCount;
    vec4 cascadeSplits[4];
} shadow;

layout(location = 0) in vec4 inWorldPosition[];

void main()
{
    if (gl_InvocationID >= int(shadow.cascadeCount))
        return;

    for (int vertex = 0; vertex < 3; ++vertex)
    {
        gl_Position = shadow.lightSpaceMatrices[gl_InvocationID] * inWorldPosition[vertex];
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }

    EndPrimitive();
}
