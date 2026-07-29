#version 450

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec2 fragWorldXZ;

layout(set = 0, binding = 0) uniform CameraBuffer
{
    mat4 view;
    mat4 proj;
} camera;

layout(push_constant) uniform GridPushConstants
{
    // x = grid height
    // y = half quad size
    // z = camera X
    // w = camera Z
    vec4 gridParams;
} grid;

// gl_VertexIndex
const vec2 quadVertices[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),

    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

void main()
{
    vec2 localXZ = quadVertices[gl_VertexIndex];

    vec2 worldXZ =
        localXZ * grid.gridParams.y +
        grid.gridParams.zw;

    vec3 worldPosition = vec3(
        worldXZ.x,
        grid.gridParams.x,
        worldXZ.y
    );

    gl_Position =
        camera.proj *
        camera.view *
        vec4(worldPosition, 1.0);

    fragWorldXZ = worldXZ;
}
