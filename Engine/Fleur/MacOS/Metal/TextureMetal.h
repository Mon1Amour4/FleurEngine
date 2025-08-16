#pragma once

#include "Renderer/Texture.h"

namespace Fleur::Renderer
{
class TextureMetal : public Texture
{
public:
    TextureMetal() = default;

    inline virtual TextureFormat GetTextureFormat() const override
    {
        return _format;
    };

private:
    TextureFormat _format;
};

class TextureViewMetal : public TextureView
{
public:
    TextureViewMetal() = default;
};
}  // namespace Fleur::Renderer
