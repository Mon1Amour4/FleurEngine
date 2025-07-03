#include "Cubemap.h"

Fuego::Graphics::Cubemap::Cubemap(const Image2D* img_1, const Image2D* img_2, const Image2D* img_3, const Image2D* img_4, const Image2D* img_5,
                                  const Image2D* img_6)
    : id(0)
{
}

Fuego::Graphics::Cubemap::Cubemap(const Image2D* equirectangular)
    : id(0)
{
}
