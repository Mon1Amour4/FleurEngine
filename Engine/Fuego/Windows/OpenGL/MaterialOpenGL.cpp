#include "MaterialOpenGL.h"

#include "ShaderObjectOpenGL.h"

namespace Fuego::Renderer
{

Material* Material::CreateMaterial(Texture* albedo)
{
    return new MaterialOpenGL(albedo);
}

MaterialOpenGL::MaterialOpenGL(Texture* albedo)
    : albedo_texture(nullptr)
{
    albedo_texture = static_cast<TextureOpenGL*>(albedo);
}

void MaterialOpenGL::Upload(ShaderObject& shader)
{
    ShaderObjectOpenGL& shader_gl = static_cast<ShaderObjectOpenGL&>(shader);
    shader_gl.UploadMaterial(*this);
}

}  // namespace Fuego::Renderer
