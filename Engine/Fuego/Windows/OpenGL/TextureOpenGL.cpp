#include "TextureOpenGL.h"

#include "glad/wgl.h"

namespace Fuego::Graphics
{

void TextureOpenGL::set_texture_parameters() const
{
    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

uint32_t TextureOpenGL::get_color_format(TextureFormat format) const
{
    switch (format)
    {
    case TextureFormat::R8:
        return GL_R8;
    case TextureFormat::RG8:
        return GL_RG8;
    case TextureFormat::RGB8:
        return GL_RGB8;
    case TextureFormat::RGBA8:
        return GL_RGBA8;

    case TextureFormat::R16F:
        return GL_R16F;
    case TextureFormat::RG16F:
        return GL_RG16F;
    case TextureFormat::RGB16F:
        return GL_RGB16F;
    case TextureFormat::RGBA16F:
        return GL_RGBA16F;

    case TextureFormat::R32F:
        return GL_R32F;
    case TextureFormat::RGBA32F:
        return GL_RGBA32F;
    }
}
uint32_t TextureOpenGL::get_pixel_format(uint16_t channels, bool inverted)
{
    switch (channels)
    {
    case 1:
        return GL_RED;
    case 2:
        return GL_RG;
    case 3:
        return inverted ? GL_BGR : GL_RGB;
    case 4:
        return inverted ? GL_BGRA : GL_RGBA;
    default:
        FU_CORE_ASSERT(false, "Unsupported channel count");
        return GL_RGBA;
    }
}
uint32_t TextureOpenGL::get_pixel_format(TextureFormat format, bool inverted)
{
    switch (format)
    {
    case TextureFormat::R8:
    case TextureFormat::R16F:
    case TextureFormat::R32F:
        return GL_RED;

    case TextureFormat::RG8:
    case TextureFormat::RG16F:
        return GL_RG;

    case TextureFormat::RGB8:
        return inverted ? GL_BGR : GL_RGB;

    case TextureFormat::RGBA8:
    case TextureFormat::RGBA16F:
    case TextureFormat::RGBA32F:
        return inverted ? GL_BGRA : GL_RGBA;

    default:
        FU_CORE_ASSERT(false, "Unsupported TextureFormat in PixelFormat");
        return GL_RGBA;
    }
}

// Texture2DOpenGL:
Texture2DOpenGL::~Texture2DOpenGL()
{
    if (base.texture_id != 0)
        glDeleteTextures(1, &base.texture_id);
}

Texture2DOpenGL::Texture2DOpenGL(std::string_view name)
    : Texture2D(name)
{
}
Texture2DOpenGL::Texture2DOpenGL(std::string_view name, TextureFormat format, unsigned char* buffer, int width, int height)
    : Texture2D(name, format, width, height)
{
    FU_CORE_ASSERT(buffer, "Texture buffer is empty");

    glCreateTextures(GL_TEXTURE_2D, 1, &base.texture_id);
    FU_CORE_ASSERT(!base.texture_id == 0, "Texture didn't create");

    uint32_t mipmap_levels = CalculateMipmapLevels(width, height);
    glTextureStorage2D(base.texture_id, mipmap_levels, base.get_color_format(format), width, height);
    glTextureSubImage2D(base.texture_id, 0, 0, 0, width, height, base.get_pixel_format(format), GL_UNSIGNED_BYTE, buffer);

    glGenerateTextureMipmap(base.texture_id);

    base.set_texture_parameters();

    // Set texture name for debug output instead of common material uniform name
    glObjectLabel(GL_TEXTURE, base.texture_id, -1, this->name.c_str());

    is_created = true;
}

void Texture2DOpenGL::PostCreate(std::shared_ptr<Fuego::Graphics::Image2D> img)
{
    const auto& image = *img.get();
    FU_CORE_ASSERT(image.Data() != nullptr && image.IsValid(), "[TextureOpenGL->PostCreate] broken image2d data");

    Texture2D::PostCreate(img);

    glCreateTextures(GL_TEXTURE_2D, 1, &base.texture_id);
    FU_CORE_ASSERT(!base.texture_id == 0, "Texture didn't create");

    uint32_t mipmap_levels = CalculateMipmapLevels(width, height);
    glTextureStorage2D(base.texture_id, mipmap_levels, base.get_color_format(format), width, height);
    glTextureSubImage2D(base.texture_id, 0, 0, 0, width, height, base.get_pixel_format(format), GL_UNSIGNED_BYTE, image.Data());

    glGenerateTextureMipmap(base.texture_id);

    // Configuration of minification/Magnification
    glTextureParameteri(base.texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(base.texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(base.texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(base.texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture name for debug output instead of common material uniform name
    glObjectLabel(GL_TEXTURE, base.texture_id, -1, this->name.c_str());

    is_created = true;
}


// CubemapTextureOpenGL:
TextureCubemapOpenGL::TextureCubemapOpenGL(const CubemapImage& cubemap)
    : TextureCubemap(cubemap.Name(), GetTextureFormat(cubemap.Channels(), cubemap.Depth()), cubemap.Width(), cubemap.Height())
{
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &base.texture_id);
    glTextureStorage2D(base.texture_id, 1, base.get_color_format(format), cubemap.Width(), cubemap.Height());

    for (uint32_t face = 0; face < 6; face++)
    {
        glTextureSubImage3D(base.texture_id,
                            0,                              // mipmap level
                            0,                              // xoffset
                            0,                              // yoffset
                            face,                           // zoffset = face index
                            cubemap.Width(),                // width
                            cubemap.Height(),               // height
                            cubemap.Depth(),                // depth = 1
                            base.get_pixel_format(format),  // format
                            GL_UNSIGNED_BYTE,
                            reinterpret_cast<const void*>(cubemap.Data())  // pointer to data
        );
    }

    base.set_texture_parameters();
}
}  // namespace Fuego::Graphics
