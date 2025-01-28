#pragma once

#include "Renderer/Texture.h"

namespace Fuego::Renderer
{
class TextureOpenGL : public Texture
{
public:
    TextureOpenGL();
    ~TextureOpenGL() = default;

    virtual TextureFormat GetTextureFormat() const override;
    inline uint16_t GetTextureUnit() const
    {
        return texture_unit;
    }

private:
    uint16_t texture_unit;
    uint32_t texture_id;
};

class TextureViewOpenGL
{
public:
    TextureViewOpenGL() = default;
    ~TextureViewOpenGL() = default;
};
}  // namespace Fuego::Renderer
