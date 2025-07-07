#pragma once

#include "CubemapTexture.h"
#include "TextureOpenGL.h"

namespace Fuego::Graphics
{
class CubemapTextureOpenGL final : public CubemapTexture, public TextureOpenGL
{
public:
    CubemapTextureOpenGL(const CubemapImage* img);
    virtual void Bind() const override
    {
    }
    virtual void UnBind() const override
    {
    }
    virtual TextureFormat GetTextureFormat() const override
    {
        return TextureFormat::NONE;
    }
};

}  // namespace Fuego::Graphics