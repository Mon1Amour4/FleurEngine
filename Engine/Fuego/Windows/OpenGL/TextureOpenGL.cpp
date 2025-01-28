#include "TextureOpenGL.h"

#include "glad/gl.h"

namespace Fuego::Renderer
{
TextureOpenGL::TextureOpenGL()
    : texture_unit(0)
    , texture_id(0)
{
    glGenTextures(1, &texture_id);
}

TextureFormat TextureOpenGL::GetTextureFormat() const
{
    return TextureFormat::R16F;
}
}  // namespace Fuego::Renderer
