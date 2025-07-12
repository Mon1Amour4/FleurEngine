#version 330 core

struct Material
{
    samplerCube skybox_cubemap;
};

uniform Material material;

out vec4 FragColor;

in vec3 TexCoords;


void main()
{    
    FragColor = texture(material.skybox_cubemap, TexCoords);
}