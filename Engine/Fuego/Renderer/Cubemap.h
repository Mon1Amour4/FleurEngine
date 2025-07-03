#pragma once
#include <array>

#include "Image2D.h"
#include "Texture.h"

namespace Fuego::Graphics
{

class Cubemap
{
public:
    Cubemap(const Image2D* img_1, const Image2D* img_2, const Image2D* img_3, const Image2D* img_4, const Image2D* img_5, const Image2D* img_6);

protected:
    std::array<const Texture*, 6> textures;
    uint32_t id;
};

}  // namespace Fuego::Graphics