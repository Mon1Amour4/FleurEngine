#version 330 core

// Layout:
layout(location = 0) in vec3 aPos;

// Uniforms:
uniform mat4 view;
uniform mat4 projection;

// In:

// Out:

void main()
{
    mat4 cameraRotation = mat4(1.0);
    cameraRotation[0] = vec4(view[0].xyz, 0.0);
    cameraRotation[1] = vec4(view[1].xyz, 0.0);
    cameraRotation[2] = vec4(view[2].xyz, 0.0);

    vec4 worldPos = cameraRotation * vec4(aPos, 1.0);
    worldPos.xy += vec2(-0.75, -0.75);
    gl_Position = projection * view * worldPos;
}