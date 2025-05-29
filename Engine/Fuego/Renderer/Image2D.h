#pragma once
#include <fupch.h>

#include "Graphics.hpp"

namespace Fuego::Graphics
{
class Image2D : public GraphicsResourceBase
{
public:
    Image2D(std::string name, unsigned char* data, int w, int h, int bpp, uint16_t channels);
    Image2D(std::string name);
    ~Image2D();

    struct Image2DPostCreateion
    {
        uint32_t width;
        uint32_t height;
        uint16_t bpp;
        uint16_t channels;
        unsigned char* data;
    };

    inline uint32_t Width() const
    {
        return width;
    }
    inline uint32_t Height() const
    {
        return height;
    }
    inline uint16_t BBP() const
    {
        return bpp;
    }
    inline uint16_t Channels() const
    {
        return channels;
    }
    unsigned char* Data() const
    {
        return data;
    }
    inline std::string_view Name() const
    {
        return name;
    }

    virtual void PostCreate(const void* settings) override;

private:
    uint32_t width;
    uint32_t height;
    uint16_t bpp;
    uint16_t channels;
    unsigned char* data;
    std::string name;
};
}  // namespace Fuego::Graphics