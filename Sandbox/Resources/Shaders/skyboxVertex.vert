#version 460

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;


layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 outTexCoord;


void main()
{
    outTexCoord = inPosition;
    mat4 viewNoTranslate = mat4(mat3(ubo.view));
    vec4 pos = ubo.proj * viewNoTranslate * vec4(inPosition, 1.0);
    gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
    //gl_Position = pos.xyww;
}