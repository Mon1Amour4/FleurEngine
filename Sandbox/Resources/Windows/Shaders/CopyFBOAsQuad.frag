#version 330 core

struct Material
{
    sampler2D albedo_texture;
};
// Uniforms:
uniform Material material;

// In:
in vec2 text_coords;

// Out:
out vec4 FragColor;

void main()
{
    vec4 Color = texture(material.albedo_texture, text_coords);

    if (Color.rgb == vec3(1.0, 1.0, 1.0))
        discard;

    FragColor = Color;
}
