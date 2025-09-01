#pragma once

#include "Renderer/Texture.h"

namespace Fleur::Graphics
{

struct TextureOpenGL : Texture
{
public:
    friend class DeviceOpenGL;

    virtual ~TextureOpenGL();

    uint32_t GetTextureUnit() const;
    const uint32_t* GetTextureID() const;

    virtual void PostCreate(ImagePostCreation& settings) override;

    TextureOpenGL(std::string_view name, std::string_view ext, uint32_t layers);
    TextureOpenGL(std::string_view name, std::string_view ext, const unsigned char* buffer, ETextureFormat format, uint32_t width, uint32_t height,
                  uint32_t layers);
    TextureOpenGL(std::string_view name, std::string_view ext, const Fleur::Graphics::CubemapInitData& images, ETextureFormat format, uint32_t width,
                  uint32_t height, uint32_t layers);

private:
    uint32_t m_TextureUnit;
    uint32_t m_TextureID;

    void SetTextureParameters() const;
    uint32_t get_color_format(ETextureFormat format) const;
    uint32_t GetPixelFormat(uint16_t channels, bool inverted = false);
    uint32_t GetPixelFormat(ETextureFormat format, bool inverted = false);

    void CreateTexture2D(const unsigned char* buffer);
    void CreateCubemap(uint32_t faceSize, const unsigned char* buffer);
    void create_cubemap_from_images(const Fleur::Graphics::CubemapInitData& images);
};

}  // namespace Fleur::Graphics
