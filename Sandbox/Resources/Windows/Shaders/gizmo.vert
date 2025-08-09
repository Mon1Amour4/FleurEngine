#version 330 core

// Layout:
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 textCoord;
layout(location = 2) in vec3 normalCoord;

// Uniforms:
uniform mat4 view;
uniform mat4 projection;

// In:

// Out:
out vec2 TextCoord;

void main()
{
    mat4 cameraRotation = mat4(1.0);
    cameraRotation[0] = vec4(view[0].x * 0.2, view[0].y * 0.2, view[0].z * 0.2, 0.0);
    cameraRotation[1] = vec4(view[1].x * 0.2, view[1].y * 0.2, view[1].z * 0.2, 0.0);
    cameraRotation[2] = vec4(view[2].x * 0.2, view[2].y * 0.2, view[2].z * 0.2, 0.0);

    vec4 worldPos = cameraRotation * vec4(aPos, 1.0);
    //worldPos.xy += vec2(-0.75, -0.75);
    gl_Position = projection * view * worldPos;
    TextCoord = textCoord;
}