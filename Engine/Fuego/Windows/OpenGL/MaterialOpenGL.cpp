#include "MaterialOpenGL.h"

#include <glad/gl.h>

#include "ShaderObjectOpenGL.h"

namespace Fuego::Graphics
{

Material* Material::CreateMaterial(const TextureBase* albedo)
{
    return new MaterialOpenGL(albedo);
}

MaterialOpenGL::MaterialOpenGL(const TextureBase* albedo)
    : albedo_texture(nullptr)
{
    albedo_texture = static_cast<const Texture2DOpenGL*>(albedo);
}

void MaterialOpenGL::Use() const
{
    glBindTexture(GL_TEXTURE_2D, albedo_texture->GetTextureID());
}

}  // namespace Fuego::Graphics
