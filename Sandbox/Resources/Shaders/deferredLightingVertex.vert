#version 450

layout(location = 0) out vec2 texCoords;

layout(set = 0, binding = 0) uniform SceneData
{
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 directionalLightColor;
    vec4 directionalLightDirectionAndIntensity;
} scene;

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    const vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
    // Keep set 0 compatible with the existing per-frame SceneData descriptor,
    // which is visible to both vertex and fragment stages.
    float sceneLayoutCompatibility = scene.view[0][0] * 0.0;
    gl_Position = vec4(positions[gl_VertexIndex], sceneLayoutCompatibility, 1.0);
    texCoords = uvs[gl_VertexIndex];
}
