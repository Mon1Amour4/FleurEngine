#pragma once

#include "../../Renderer/TextureUtills.h"
#include "Renderer/Texture.h"

namespace Fuego::Graphics
{

struct TextureOpenGL
{
    TextureOpenGL() = default;
    virtual ~TextureOpenGL() = default;

    void set_texture_parameters() const;
    uint32_t get_color_format(TextureFormat format) const;
    uint32_t get_pixel_format(uint16_t channels, bool inverted = false);
    uint32_t get_pixel_format(TextureFormat format, bool inverted = false);
    uint16_t texture_unit = 0;
    uint32_t texture_id = 0;
};

class Texture2DOpenGL : public Texture2D
{
public:
    friend class DeviceOpenGL;
    Texture2DOpenGL(std::string_view name);
    Texture2DOpenGL(std::string_view name, TextureFormat format, unsigned char* buffer, int width, int height);

    virtual ~Texture2DOpenGL() override;

    virtual void PostCreate(std::shared_ptr<Fuego::Graphics::Image2D> img) override;
    uint32_t GetTextureUnit() const
    {
        return base.texture_unit;
    }
    uint32_t GetTextureID() const
    {
        return base.texture_id;
    }

private:
    TextureOpenGL base;
};

class TextureCubemapOpenGL : public TextureCubemap
{
public:
    friend class DeviceOpenGL;
    TextureCubemapOpenGL(const CubemapImage& cubemap);

private:
    TextureOpenGL base;
};

class TextureViewOpenGL
{
public:
    TextureViewOpenGL() = default;
    ~TextureViewOpenGL() = default;
};

}  // namespace Fuego::Graphics
