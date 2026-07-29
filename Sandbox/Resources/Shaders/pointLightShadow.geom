#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(set = 1, binding = 0) uniform ShadowMatrices
{
    mat4 viewProjection[6];
} lightMatrices;

void main()
{
    for (int face = 0; face < 6; ++face)
    {
        gl_Layer = face;

        for (int vertex = 0; vertex < 3; ++vertex)
        {
            gl_Position = lightMatrices.viewProjection[face] * gl_in[vertex].gl_Position;
            EmitVertex();
        }

        EndPrimitive();
    }
}
