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

void Fuego::Graphics::Image2D::PostCreate(const void* settings)
{
    GraphicsResourceBase::PostCreate(nullptr);

    const auto& image_settings = *reinterpret_cast<const Image2DPostCreateion*>(settings);
    FU_CORE_ASSERT(image_settings.data, "[Image2D] data is nullptr");
    width = image_settings.width;
    height = image_settings.height;
    bpp = image_settings.bpp;
    channels = image_settings.channels;
    data = image_settings.data;
}
