#include "TextureOpenGL.h"

#include "glad/wgl.h"

Fleur::Graphics::TextureOpenGL::TextureOpenGL(std::string_view name, std::string_view ext, uint32_t layers)
    : Texture(name, ext, layers)
    , texture_unit(0)
    , texture_id(0)
{
}

Fleur::Graphics::TextureOpenGL::TextureOpenGL(std::string_view name, std::string_view ext, const unsigned char* buffer, TextureFormat format, uint32_t width,
                                              uint32_t height, uint32_t layers)
    : Texture(name, ext, format, width, height, layers)
    , texture_unit(0)
    , texture_id(0)
{
    if (layers == 1)
        create_texture_2d(buffer);
    else if (layers == 6)
        create_cubemap(width, buffer);

    // Set texture name for debug output instead of common material uniform name
    glObjectLabel(GL_TEXTURE, texture_id, -1, this->m_Name.c_str());

    if (buffer)
        m_IsCreated = true;
}

Fleur::Graphics::TextureOpenGL::TextureOpenGL(std::string_view name, std::string_view ext, const Fleur::Graphics::CubemapInitData& images, TextureFormat format,
                                              uint32_t width, uint32_t height, uint32_t layers)
    : Texture(name, ext, format, width, height, layers)
    , texture_unit(0)
    , texture_id(0)
{
    create_cubemap_from_images(images);
    glObjectLabel(GL_TEXTURE, texture_id, -1, this->m_Name.c_str());

    m_IsCreated = true;
}

Fleur::Graphics::TextureOpenGL::~TextureOpenGL()
{
    if (texture_id != 0)
        glDeleteTextures(1, &texture_id);
}

void Fleur::Graphics::TextureOpenGL::PostCreate(ImagePostCreation& settings)
{
    Texture::PostCreate(settings);

    FL_CORE_ASSERT(settings.data, "[TextureOpenGL->PostCreate] invalid post create settings");

    if (m_Layers == 1)
        create_texture_2d(reinterpret_cast<const unsigned char*>(settings.data));
    else if (m_Layers == 6)
        create_cubemap(m_Width, reinterpret_cast<const unsigned char*>(settings.data));

    glGenerateTextureMipmap(texture_id);

    set_texture_parameters();

    // Set texture name for debug output instead of common material uniform name
    glObjectLabel(GL_TEXTURE, texture_id, -1, this->m_Name.c_str());

    m_IsCreated = true;
}

void Fleur::Graphics::TextureOpenGL::set_texture_parameters() const
{
    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

uint32_t Fleur::Graphics::TextureOpenGL::get_color_format(TextureFormat format) const
{
    switch (format)
    {
    // 8-bit UNORM
    case TextureFormat::R8:
        return GL_R8;
    case TextureFormat::RG8:
        return GL_RG8;
    case TextureFormat::RGB8:
        return GL_RGB8;
    case TextureFormat::RGBA8:
        return GL_RGBA8;

    // 16-bit float
    case TextureFormat::R16F:
        return GL_R16F;
    case TextureFormat::RG16F:
        return GL_RG16F;
    case TextureFormat::RGB16F:
        return GL_RGB16F;
    case TextureFormat::RGBA16F:
        return GL_RGBA16F;

    // 32-bit float
    case TextureFormat::R32F:
        return GL_R32F;
    case TextureFormat::RG32F:
        return GL_RG32F;
    case TextureFormat::RGB32F:
        return GL_RGB32F;
    case TextureFormat::RGBA32F:
        return GL_RGBA32F;

    // Depth
    case TextureFormat::DEPTH16:
        return GL_DEPTH_COMPONENT16;
    case TextureFormat::DEPTH24:
        return GL_DEPTH_COMPONENT24;
    case TextureFormat::DEPTH32:
        return GL_DEPTH_COMPONENT32;
    case TextureFormat::DEPTH32F:
        return GL_DEPTH_COMPONENT32F;

    // Stencil
    case TextureFormat::STENCIL8:
        return GL_STENCIL_INDEX8;

    // Depth-stencil
    case TextureFormat::DEPTH24STENCIL8:
        return GL_DEPTH24_STENCIL8;
    case TextureFormat::DEPTH24FSTENCIL8F:
        return GL_DEPTH32F_STENCIL8;

    default:
        FL_CORE_ASSERT(false, "Unsupported texture format");
        return GL_RGBA8;
    }
}
uint32_t Fleur::Graphics::TextureOpenGL::get_pixel_format(uint16_t channels, bool inverted)
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
        FL_CORE_ASSERT(false, "Unsupported channel count");
        return GL_RGBA;
    }
}
uint32_t Fleur::Graphics::TextureOpenGL::get_pixel_format(TextureFormat format, bool inverted)
{
    switch (format)
    {
    // Single channel
    case TextureFormat::R8:
    case TextureFormat::R16F:
    case TextureFormat::R32F:
        return GL_RED;

    // Two channel
    case TextureFormat::RG8:
    case TextureFormat::RG16F:
    case TextureFormat::RG32F:
        return GL_RG;

    // Three channel
    case TextureFormat::RGB8:
    case TextureFormat::RGB16F:
    case TextureFormat::RGB32F:
        return inverted ? GL_BGR : GL_RGB;

    // Four channel
    case TextureFormat::RGBA8:
    case TextureFormat::RGBA16F:
    case TextureFormat::RGBA32F:
        return inverted ? GL_BGRA : GL_RGBA;

    // Depth
    case TextureFormat::DEPTH16:
    case TextureFormat::DEPTH24:
    case TextureFormat::DEPTH32:
    case TextureFormat::DEPTH32F:
        return GL_DEPTH_COMPONENT;

    // Stencil
    case TextureFormat::STENCIL8:
        return GL_STENCIL_INDEX;

    // Depth-stencil
    case TextureFormat::DEPTH24STENCIL8:
    case TextureFormat::DEPTH24FSTENCIL8F:
        return GL_DEPTH_STENCIL;

    default:
        FL_CORE_ASSERT(false, "Unsupported TextureFormat in PixelFormat");
        return GL_RGBA;
    }
}

void Fleur::Graphics::TextureOpenGL::create_texture_2d(const unsigned char* buffer)
{
    FL_CORE_ASSERT(m_Layers == 1, "");

    glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);
    FL_CORE_ASSERT(texture_id != 0, "Failed to create 2D texture");

    uint32_t mipmap_levels = calculate_mipmap_level(m_Width, m_Height);
    glTextureStorage2D(texture_id, mipmap_levels, get_color_format(m_Format), m_Width, m_Height);

    if(buffer)
        glTextureSubImage2D(texture_id, 0, 0, 0, m_Width, m_Height, get_pixel_format(m_Format), GL_UNSIGNED_BYTE, buffer);

    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Fleur::Graphics::TextureOpenGL::create_cubemap(uint32_t face_size, const unsigned char* buffer)
{
    FL_CORE_ASSERT(m_Layers == 6, "");
    const Fleur::Graphics::Image2D* images = reinterpret_cast<const Fleur::Graphics::Image2D*>(buffer);
    FL_CORE_ASSERT(images, "");

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture_id);
    FL_CORE_ASSERT(texture_id != 0, "Failed to create cubemap texture");


    uint32_t mipmap_levels = calculate_mipmap_level(face_size, face_size);
    glTextureStorage2D(texture_id, mipmap_levels, get_color_format(m_Format), face_size, face_size);


    for (uint32_t face = 0; face < 6; ++face)
    {
        const auto& img = images + face;
        FL_CORE_ASSERT(img && img->Data(), "Cubemap image is null or has no data");

        glTextureSubImage3D(texture_id,
                            0,                         // mipmap level
                            0,                         // xoffset
                            0,                         // yoffset
                            face,                      // zoffset = face index
                            m_Width,                     // width
                            m_Height,                    // height
                            1,                         // depth = 1
                            get_pixel_format(m_Format),  // format
                            GL_UNSIGNED_BYTE,
                            reinterpret_cast<const void*>(img->Data())  // pointer to data
        );
    }


    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Fleur::Graphics::TextureOpenGL::create_cubemap_from_images(const Fleur::Graphics::CubemapInitData& images)
{
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture_id);
    FL_CORE_ASSERT(texture_id != 0, "Failed to create cubemap texture");

    uint32_t mipmap_levels = calculate_mipmap_level(m_Width, m_Height);
    glTextureStorage2D(texture_id, mipmap_levels, get_color_format(m_Format), m_Width, m_Height);

    const std::shared_ptr<Fleur::Graphics::Image2D> faces[6] = {images.right, images.left, images.top, images.bottom, images.back, images.front};

    for (uint32_t face = 0; face < 6; ++face)
    {
        const auto& img = faces[face];
        FL_CORE_ASSERT(img && img->Data(), "Cubemap image is null or has no data");

        glTextureSubImage3D(texture_id,
                            0,                         // mipmap level
                            0,                         // xoffset
                            0,                         // yoffset
                            face,                      // zoffset = face index
                            m_Width,                     // width
                            m_Height,                    // height
                            1,                         // depth = 1
                            get_pixel_format(m_Format),  // format
                            GL_UNSIGNED_BYTE,
                            reinterpret_cast<const void*>(img->Data())  // pointer to data
        );
    }
    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

uint32_t Fleur::Graphics::TextureOpenGL::GetTextureUnit() const
{
    return texture_unit;
}

const uint32_t* Fleur::Graphics::TextureOpenGL::GetTextureID() const
{
    return &texture_id;
}
