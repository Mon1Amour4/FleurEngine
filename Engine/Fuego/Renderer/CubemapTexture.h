#pragma once

#include "Image2D.h"
#include "Texture.h"

namespace Fuego::Graphics
{

class CubemapTexture : public Texture
{
public:
    CubemapTexture(const CubemapImage* equirectangular);

protected:
    uint32_t id;
};

}  // namespace Fuego::Graphics