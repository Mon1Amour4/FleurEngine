#pragma once

#include "../../Renderer/TextureUtills.h"
#include "Renderer/Texture.h"

namespace Fuego::Graphics
{

struct TextureOpenGL : Texture
{
public:
    friend class DeviceOpenGL;

    virtual ~TextureOpenGL();

    uint32_t GetTextureUnit() const;
    uint32_t GetTextureID() const;

    virtual void PostCreate(ImagePostCreation& settings) override;

    TextureOpenGL(std::string_view name, std::string_view ext, uint32_t layers);
    TextureOpenGL(std::string_view name, std::string_view ext, const unsigned char* buffer, TextureFormat format, uint32_t width, uint32_t height,
                  uint32_t layers);
    TextureOpenGL(std::string_view name, std::string_view ext, const Fuego::Graphics::CubemapInitData& images, TextureFormat format,
                  uint32_t width, uint32_t height,
                  uint32_t layers);

private:
    uint32_t texture_unit;
    uint32_t texture_id;

    void set_texture_parameters() const;
    uint32_t get_color_format(TextureFormat format) const;
    uint32_t get_pixel_format(uint16_t channels, bool inverted = false);
    uint32_t get_pixel_format(TextureFormat format, bool inverted = false);

    void create_texture_2d(const unsigned char* buffer);
    void create_cubemap(const unsigned char* buffer);
    void create_cubemap_from_images(const Fuego::Graphics::CubemapInitData& images);
};

}  // namespace Fuego::Graphics
