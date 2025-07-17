#version 330 core
// Layout:
layout(location = 0) in vec3 VertexPosition;

out vec3 TexCoords; 

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = VertexPosition;
    gl_Position = projection * view * vec4(VertexPosition, 1.0);
}  