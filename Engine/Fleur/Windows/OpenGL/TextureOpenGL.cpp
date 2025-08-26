#include "TextureOpenGL.h"

#include "glad/wgl.h"

Fleur::Graphics::TextureOpenGL::TextureOpenGL(std::string_view name, std::string_view ext, uint32_t layers)
    : Texture(name, ext, layers)
    , m_TextureUnit(0)
    , m_TextureID(0)
{
}

Fleur::Graphics::TextureOpenGL::TextureOpenGL(std::string_view name, std::string_view ext, const unsigned char* buffer, ETextureFormat format, uint32_t width,
                                              uint32_t height, uint32_t layers)
    : Texture(name, ext, format, width, height, layers)
    , m_TextureUnit(0)
    , m_TextureID(0)
{
    if (layers == 1)
        CreateTexture2D(buffer);
    else if (layers == 6)
        CreateCubemap(width, buffer);

    // Set texture name for debug output instead of common material uniform name
    glObjectLabel(GL_TEXTURE, m_TextureID, -1, this->m_Name.c_str());

    if (buffer)
        m_IsCreated = true;
}

Fleur::Graphics::TextureOpenGL::TextureOpenGL(std::string_view name, std::string_view ext, const Fleur::Graphics::CubemapInitData& images, ETextureFormat format,
                                              uint32_t width, uint32_t height, uint32_t layers)
    : Texture(name, ext, format, width, height, layers)
    , m_TextureUnit(0)
    , m_TextureID(0)
{
    create_cubemap_from_images(images);
    glObjectLabel(GL_TEXTURE, m_TextureID, -1, this->m_Name.c_str());

    m_IsCreated = true;
}

Fleur::Graphics::TextureOpenGL::~TextureOpenGL()
{
    if (m_TextureID != 0)
        glDeleteTextures(1, &m_TextureID);
}

void Fleur::Graphics::TextureOpenGL::PostCreate(ImagePostCreation& settings)
{
    Texture::PostCreate(settings);

    FL_CORE_ASSERT(settings.data, "[TextureOpenGL->PostCreate] invalid post create settings");

    if (m_Layers == 1)
        CreateTexture2D(reinterpret_cast<const unsigned char*>(settings.data));
    else if (m_Layers == 6)
        CreateCubemap(m_Width, reinterpret_cast<const unsigned char*>(settings.data));

    glGenerateTextureMipmap(m_TextureID);

    SetTextureParameters();

    // Set texture name for debug output instead of common material uniform name
    glObjectLabel(GL_TEXTURE, m_TextureID, -1, this->m_Name.c_str());

    m_IsCreated = true;
}

void Fleur::Graphics::TextureOpenGL::SetTextureParameters() const
{
    glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

uint32_t Fleur::Graphics::TextureOpenGL::get_color_format(ETextureFormat format) const
{
    switch (format)
    {
    // 8-bit UNORM
    case ETextureFormat::R8:
        return GL_R8;
    case ETextureFormat::RG8:
        return GL_RG8;
    case ETextureFormat::RGB8:
        return GL_RGB8;
    case ETextureFormat::RGBA8:
        return GL_RGBA8;

    // 16-bit float
    case ETextureFormat::R16F:
        return GL_R16F;
    case ETextureFormat::RG16F:
        return GL_RG16F;
    case ETextureFormat::RGB16F:
        return GL_RGB16F;
    case ETextureFormat::RGBA16F:
        return GL_RGBA16F;

    // 32-bit float
    case ETextureFormat::R32F:
        return GL_R32F;
    case ETextureFormat::RG32F:
        return GL_RG32F;
    case ETextureFormat::RGB32F:
        return GL_RGB32F;
    case ETextureFormat::RGBA32F:
        return GL_RGBA32F;

    // Depth
    case ETextureFormat::DEPTH16:
        return GL_DEPTH_COMPONENT16;
    case ETextureFormat::DEPTH24:
        return GL_DEPTH_COMPONENT24;
    case ETextureFormat::DEPTH32:
        return GL_DEPTH_COMPONENT32;
    case ETextureFormat::DEPTH32F:
        return GL_DEPTH_COMPONENT32F;

    // Stencil
    case ETextureFormat::STENCIL8:
        return GL_STENCIL_INDEX8;

    // Depth-stencil
    case ETextureFormat::DEPTH24STENCIL8:
        return GL_DEPTH24_STENCIL8;
    case ETextureFormat::DEPTH24FSTENCIL8F:
        return GL_DEPTH32F_STENCIL8;

    default:
        FL_CORE_ASSERT(false, "Unsupported texture format");
        return GL_RGBA8;
    }
}
uint32_t Fleur::Graphics::TextureOpenGL::GetPixelFormat(uint16_t channels, bool inverted)
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
uint32_t Fleur::Graphics::TextureOpenGL::GetPixelFormat(ETextureFormat format, bool inverted)
{
    switch (format)
    {
    // Single channel
    case ETextureFormat::R8:
    case ETextureFormat::R16F:
    case ETextureFormat::R32F:
        return GL_RED;

    // Two channel
    case ETextureFormat::RG8:
    case ETextureFormat::RG16F:
    case ETextureFormat::RG32F:
        return GL_RG;

    // Three channel
    case ETextureFormat::RGB8:
    case ETextureFormat::RGB16F:
    case ETextureFormat::RGB32F:
        return inverted ? GL_BGR : GL_RGB;

    // Four channel
    case ETextureFormat::RGBA8:
    case ETextureFormat::RGBA16F:
    case ETextureFormat::RGBA32F:
        return inverted ? GL_BGRA : GL_RGBA;

    // Depth
    case ETextureFormat::DEPTH16:
    case ETextureFormat::DEPTH24:
    case ETextureFormat::DEPTH32:
    case ETextureFormat::DEPTH32F:
        return GL_DEPTH_COMPONENT;

    // Stencil
    case ETextureFormat::STENCIL8:
        return GL_STENCIL_INDEX;

    // Depth-stencil
    case ETextureFormat::DEPTH24STENCIL8:
    case ETextureFormat::DEPTH24FSTENCIL8F:
        return GL_DEPTH_STENCIL;

    default:
        FL_CORE_ASSERT(false, "Unsupported TextureFormat in PixelFormat");
        return GL_RGBA;
    }
}

void Fleur::Graphics::TextureOpenGL::CreateTexture2D(const unsigned char* buffer)
{
    FL_CORE_ASSERT(m_Layers == 1, "");

    glCreateTextures(GL_TEXTURE_2D, 1, &m_TextureID);
    FL_CORE_ASSERT(m_TextureID != 0, "Failed to create 2D texture");

    uint32_t mipmap_levels = calculate_mipmap_level(m_Width, m_Height);
    glTextureStorage2D(m_TextureID, mipmap_levels, get_color_format(m_Format), m_Width, m_Height);

    if(buffer)
        glTextureSubImage2D(m_TextureID, 0, 0, 0, m_Width, m_Height, GetPixelFormat(m_Format), GL_UNSIGNED_BYTE, buffer);

    glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Fleur::Graphics::TextureOpenGL::CreateCubemap(uint32_t faceSize, const unsigned char* buffer)
{
    FL_CORE_ASSERT(m_Layers == 6, "");
    const Fleur::Graphics::Image2D* images = reinterpret_cast<const Fleur::Graphics::Image2D*>(buffer);
    FL_CORE_ASSERT(images, "");

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_TextureID);
    FL_CORE_ASSERT(m_TextureID != 0, "Failed to create cubemap texture");


    uint32_t mipmapLevels = calculate_mipmap_level(faceSize, faceSize);
    glTextureStorage2D(m_TextureID, mipmapLevels, get_color_format(m_Format), faceSize, faceSize);


    for (uint32_t face = 0; face < 6; ++face)
    {
        const auto& img = images + face;
        FL_CORE_ASSERT(img && img->Data(), "Cubemap image is null or has no data");

        glTextureSubImage3D(m_TextureID,
                            0,                         // mipmap level
                            0,                         // xoffset
                            0,                         // yoffset
                            face,                      // zoffset = face index
                            m_Width,                     // width
                            m_Height,                    // height
                            1,                         // depth = 1
                            GetPixelFormat(m_Format),  // format
                            GL_UNSIGNED_BYTE,
                            reinterpret_cast<const void*>(img->Data())  // pointer to data
        );
    }


    glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Fleur::Graphics::TextureOpenGL::create_cubemap_from_images(const Fleur::Graphics::CubemapInitData& images)
{
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_TextureID);
    FL_CORE_ASSERT(m_TextureID != 0, "Failed to create cubemap texture");

    uint32_t mipmapLevels = calculate_mipmap_level(m_Width, m_Height);
    glTextureStorage2D(m_TextureID, mipmapLevels, get_color_format(m_Format), m_Width, m_Height);

    const std::shared_ptr<Fleur::Graphics::Image2D> faces[6] = {images.right, images.left, images.top, images.bottom, images.back, images.front};

    for (uint32_t face = 0; face < 6; ++face)
    {
        const auto& img = faces[face];
        FL_CORE_ASSERT(img && img->Data(), "Cubemap image is null or has no data");

        glTextureSubImage3D(m_TextureID,
                            0,                         // mipmap level
                            0,                         // xoffset
                            0,                         // yoffset
                            face,                      // zoffset = face index
                            m_Width,                     // width
                            m_Height,                    // height
                            1,                         // depth = 1
                            GetPixelFormat(m_Format),  // format
                            GL_UNSIGNED_BYTE,
                            reinterpret_cast<const void*>(img->Data())  // pointer to data
        );
    }
    glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

uint32_t Fleur::Graphics::TextureOpenGL::GetTextureUnit() const
{
    return m_TextureUnit;
}

const uint32_t* Fleur::Graphics::TextureOpenGL::GetTextureID() const
{
    return &m_TextureID;
}
