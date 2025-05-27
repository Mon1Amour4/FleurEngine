#include "Image2D.h"

Fuego::Graphics::Image2D::Image2D(std::string name, unsigned char* data, int w, int h, int bpp, uint16_t channels)
    : name(name)
    , data(data)
    , width(w)
    , height(h)
    , bpp(bpp)
    , channels(channels)
{
    FU_CORE_ASSERT(bpp > 0 && channels > 0, "Invalid Image data");
    is_created = true;
}
Fuego::Graphics::Image2D::Image2D(std::string name)
    : name(name)
    , data(nullptr)
    , width(0)
    , height(0)
    , bpp(0)
    , channels(0)
{
    is_created = false;
}
Fuego::Graphics::Image2D::~Image2D()
{
    delete data;
}

void Fuego::Graphics::Image2D::PostCreate(Image2DPostCreateion& settings)
{
    FU_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");
    width = settings.width;
    height = settings.height;
    bpp = settings.bpp;
    channels = settings.channels;
    data = settings.data;
    is_created = true;
}