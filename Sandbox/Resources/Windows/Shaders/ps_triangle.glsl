#version 330 core

struct Material
{
    sampler2D albedo_text;
};
// Uniforms:
uniform Material material;
// In:
in vec2 text_coords;
// Out:
out vec4 FragColor;

void main()
{
    FragColor = texture(material.albedo_text, text_coords);
}
