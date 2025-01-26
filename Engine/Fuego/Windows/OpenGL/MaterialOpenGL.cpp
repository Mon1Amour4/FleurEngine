#include "MaterialOpenGL.h"

namespace Fuego::Renderer
{
MaterialOpenGL::MaterialOpenGL(uint16_t albedo)
    : albedo_texture(albedo)
{
}

void MaterialOpenGL::Bind(Shader& shader)
{
}
}  // namespace Fuego::Renderer