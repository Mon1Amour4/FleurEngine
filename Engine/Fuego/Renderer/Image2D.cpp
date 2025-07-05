#include "Image2D.h"

#include "Color.h"
#include "Services/ServiceLocator.h"

Fuego::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext, unsigned char* data, int w, int h, uint16_t channels, uint16_t depth)
    : name(name)
    , ext(ext)
    , data(data)
    , width(w)
    , height(h)
    , depth(depth)
    , channels(channels)
    , size_bytes(w * h * channels * depth)
{
    FU_CORE_ASSERT(depth > 0 && channels > 0, "Invalid Image data");
    is_created = true;
}

Fuego::Graphics::Image2D::Image2D(std::string_view name, std::string_view ext)
    : name(name)
    , ext(ext)
    , data(nullptr)
    , width(0)
    , height(0)
    , depth(0)
    , channels(0)
    , size_bytes(0)
{
    is_created = false;
}

Fuego::Graphics::Image2D::~Image2D()
{
    ServiceLocator::instance().GetService<Fuego::AssetsManager>()->FreeImage2D(data);
}

void Fuego::Graphics::Image2D::PostCreate(Image2DPostCreateion& settings)
{
    FU_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");
    width = settings.width;
    height = settings.height;
    depth = settings.depth;
    channels = settings.channels;
    data = settings.data;
    is_created = true;
    size_bytes = width * height * channels * depth;
}
