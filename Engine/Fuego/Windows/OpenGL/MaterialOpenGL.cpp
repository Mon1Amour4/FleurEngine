#include "MaterialOpenGL.h"

#include <glad/gl.h>

#include "ShaderObjectOpenGL.h"

namespace Fuego::Graphics
{

MaterialOpenGL::MaterialOpenGL(const Texture* albedo)
    : albedo_texture(nullptr)
{
    albedo_texture = static_cast<const TextureOpenGL*>(albedo);
}

void MaterialOpenGL::Use() const
{
    glBindTexture(GL_TEXTURE_2D, albedo_texture->GetTextureID());
}

}  // namespace Fuego::Graphics
