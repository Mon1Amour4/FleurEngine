#version 330 core
// Layout:
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTextCoord;
layout(location = 2) in vec3 aNormal;  
// Uniforms:
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
// In:

// Out:
out vec2 text_coords;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    text_coords = aTextCoord;
}
