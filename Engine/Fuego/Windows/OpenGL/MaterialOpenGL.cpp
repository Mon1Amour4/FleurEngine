#include "MaterialOpenGL.h"

namespace Fuego::Renderer
{

Material* Material::CreateMaterial(uint32_t albedo)
{
    return new MaterialOpenGL(albedo);
}

MaterialOpenGL::MaterialOpenGL(uint32_t albedo)
    : albedo_texture(albedo)
{
}

void MaterialOpenGL::Bind(Shader& shader)
{
}
}  // namespace Fuego::Renderer
