#include "TextureOpenGL.h"

#include "glad/wgl.h"

Fuego::Graphics::TextureOpenGL::TextureOpenGL(std::string_view name, std::string_view ext, uint32_t layers)
    : Texture(name, ext, layers)
    , texture_unit(0)
    , texture_id(0)
{
}

Fuego::Graphics::TextureOpenGL::TextureOpenGL(std::string_view name, std::string_view ext, const unsigned char* buffer, TextureFormat format, uint32_t width,
                                              uint32_t height, uint32_t layers)
    : Texture(name, ext, format, width, height, layers)
    , texture_unit(0)
    , texture_id(0)
{
    FU_CORE_ASSERT(buffer, "");

    if (layers == 1)
        create_texture_2d(buffer);
    else if (layers == 6)
        create_cubemap(buffer);

    // Set texture name for debug output instead of common material uniform name
    glObjectLabel(GL_TEXTURE, texture_id, -1, this->name.c_str());

    is_created = true;
}

Fuego::Graphics::TextureOpenGL::TextureOpenGL(std::string_view name, std::string_view ext, const Fuego::Graphics::CubemapInitData& images, TextureFormat format,
                                              uint32_t width, uint32_t height, uint32_t layers)
    : Texture(name, ext, format, width, height, layers)
    , texture_unit(0)
    , texture_id(0)
{
    create_cubemap_from_images(images);
    glObjectLabel(GL_TEXTURE, texture_id, -1, this->name.c_str());

    is_created = true;
}

Fuego::Graphics::TextureOpenGL::~TextureOpenGL()
{
    if (texture_id != 0)
        glDeleteTextures(1, &texture_id);
}

void Fuego::Graphics::TextureOpenGL::PostCreate(ImagePostCreation& settings)
{
    Texture::PostCreate(settings);

    FU_CORE_ASSERT(settings.data, "[TextureOpenGL->PostCreate] invalid post create settings");

    if (layers == 1)
        create_texture_2d(reinterpret_cast<const unsigned char*>(settings.data));
    else if (layers == 6)
        create_cubemap(reinterpret_cast<const unsigned char*>(settings.data));

    glGenerateTextureMipmap(texture_id);

    set_texture_parameters();

    // Set texture name for debug output instead of common material uniform name
    glObjectLabel(GL_TEXTURE, texture_id, -1, this->name.c_str());

    is_created = true;
}

void Fuego::Graphics::TextureOpenGL::set_texture_parameters() const
{
    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

uint32_t Fuego::Graphics::TextureOpenGL::get_color_format(TextureFormat format) const
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
    default:
        FU_CORE_ASSERT(false, "Unsupported texture format");
        return GL_RGBA8;
    }
}
uint32_t Fuego::Graphics::TextureOpenGL::get_pixel_format(uint16_t channels, bool inverted)
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
uint32_t Fuego::Graphics::TextureOpenGL::get_pixel_format(TextureFormat format, bool inverted)
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

void Fuego::Graphics::TextureOpenGL::create_texture_2d(const unsigned char* buffer)
{
    FU_CORE_ASSERT(layers == 1, "");

    glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);
    FU_CORE_ASSERT(texture_id != 0, "Failed to create 2D texture");

    uint32_t mipmap_levels = calculate_mipmap_level(width, height);
    glTextureStorage2D(texture_id, mipmap_levels, get_color_format(format), width, height);

    glTextureSubImage2D(texture_id, 0, 0, 0, width, height, get_pixel_format(format), GL_UNSIGNED_BYTE, buffer);

    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Fuego::Graphics::TextureOpenGL::create_cubemap(const unsigned char* buffer)
{
    FU_CORE_ASSERT(layers == 6, "");
    const Fuego::Graphics::Image2D* images = reinterpret_cast<const Fuego::Graphics::Image2D*>(buffer);
    FU_CORE_ASSERT(images, "");

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture_id);
    FU_CORE_ASSERT(texture_id != 0, "Failed to create cubemap texture");

    uint32_t face_size = width / 4;
    uint32_t mipmap_levels = calculate_mipmap_level(face_size, face_size);
    glTextureStorage2D(texture_id, mipmap_levels, get_color_format(format), face_size, face_size);

    auto get_stride = [this](TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::RGB8:
            return 3;
        case TextureFormat::RGBA8:
            return 4;
        }
    };
    uint32_t stride = get_stride(format);

    auto upload_face =
        [this](const unsigned char* buffer, uint32_t start_x, uint32_t start_y, uint32_t face_size, uint32_t width, uint32_t stride, uint32_t face)
    {
        uint32_t row_stride = width * stride;
        const uint8_t* base = buffer + (start_y * width + start_x) * stride;

        for (size_t row = 0; row < face_size; row++)
        {
            const void* row_ptr = base + row * row_stride;
            glTextureSubImage3D(texture_id,
                                0,                         // mipmap level
                                0,                         // xoffset
                                row,                       // yoffset
                                face,                      // zoffset = face index
                                face_size,                 // width
                                1,                         // height
                                1,                         // depth = 1
                                get_pixel_format(format),  // format
                                GL_UNSIGNED_BYTE,
                                reinterpret_cast<const void*>(row_ptr)  // pointer to data
            );
        }
    };
    // clang-format off
    upload_face(buffer, face_size,      0,              face_size, width, stride, 2);  // top
    upload_face(buffer, 0,              face_size,      face_size, width, stride, 1);  // left
    upload_face(buffer, face_size,      face_size,      face_size, width, stride, 4);  // front
    upload_face(buffer, face_size * 2,  face_size,      face_size, width, stride, 0);  // right
    upload_face(buffer, face_size * 3,  face_size,      face_size, width, stride, 5);  // back
    upload_face(buffer, face_size,      face_size * 2,  face_size, width, stride, 3);  // bottom
    // clang-format on

    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Fuego::Graphics::TextureOpenGL::create_cubemap_from_images(const Fuego::Graphics::CubemapInitData& images)
{
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture_id);
    FU_CORE_ASSERT(texture_id != 0, "Failed to create cubemap texture");

    uint32_t mipmap_levels = calculate_mipmap_level(width, height);
    glTextureStorage2D(texture_id, mipmap_levels, get_color_format(format), width, height);

    const std::shared_ptr<Fuego::Graphics::Image2D> faces[6] = {images.right, images.left, images.top, images.bottom, images.back, images.front};

    for (uint32_t face = 0; face < 6; ++face)
    {
        const auto& img = faces[face];
        FU_CORE_ASSERT(img && img->Data(), "Cubemap image is null or has no data");

        glTextureSubImage3D(texture_id,
                            0,                         // mipmap level
                            0,                         // xoffset
                            0,                         // yoffset
                            face,                      // zoffset = face index
                            width,                     // width
                            height,                    // height
                            1,                         // depth = 1
                            get_pixel_format(format),  // format
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

uint32_t Fuego::Graphics::TextureOpenGL::GetTextureUnit() const
{
    return texture_unit;
}

uint32_t Fuego::Graphics::TextureOpenGL::GetTextureID() const
{
    return texture_id;
}
