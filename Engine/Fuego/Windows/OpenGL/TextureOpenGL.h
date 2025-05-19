#pragma once

#include "Renderer/Texture.h"

namespace Fuego::Graphics
{
class TextureOpenGL final : public Texture
{
public:
    TextureOpenGL() = default;
    ~TextureOpenGL();

    virtual void Bind() const override;
    virtual void UnBind() const override;

    virtual TextureFormat GetTextureFormat() const override;
    inline uint16_t GetTextureUnit() const
    {
        return texture_unit;
    }
    inline uint32_t GetTextureID() const
    {
        return texture_id;
    }

private:
    uint16_t texture_unit;
    uint32_t texture_id;
    friend class DeviceOpenGL;

    TextureOpenGL(std::string_view name, unsigned char* buffer, int width, int height);
};

class TextureViewOpenGL
{
public:
    TextureViewOpenGL() = default;
    ~TextureViewOpenGL() = default;
};
}  // namespace Fuego::Graphics
