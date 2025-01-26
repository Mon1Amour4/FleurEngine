#include "MaterialOpenGL.h"

namespace Fuego::Renderer
{

Material* Material::CreateMaterial(uint16_t albedo)
{
    return new MaterialOpenGL(albedo);
}

MaterialOpenGL::MaterialOpenGL(uint16_t albedo)
    : albedo_texture(albedo)
{
}

void MaterialOpenGL::Bind(Shader& shader)
{
}
}  // namespace Fuego::Renderer
