#pragma once

#include "Color.h"
#include "Image2D.h"

namespace Fuego::Graphics
{

struct CubemapInitData
{
    const std::shared_ptr<Fuego::Graphics::Image2D> right;
    const std::shared_ptr<Fuego::Graphics::Image2D> left;
    const std::shared_ptr<Fuego::Graphics::Image2D> top;
    const std::shared_ptr<Fuego::Graphics::Image2D> bottom;
    const std::shared_ptr<Fuego::Graphics::Image2D> back;
    const std::shared_ptr<Fuego::Graphics::Image2D> front;
};

class Device;

enum class TextureFormat
{
    // 8-bit unsigned normalized integer formats
    R8,     // 1 channel, 8-bit
    RG8,    // 2 channel, 8-bit
    RGB8,   // 3 channel, 8-bit
    RGBA8,  // 4 channel, 8-bit

    // 16-bit float formats
    R16F,     // 1 channel, 16-bit float
    RG16F,    // 2 channel, 16-bit float
    RGB16F,   // 3 channel, 16-bit float
    RGBA16F,  // 4 channel, 16-bit float

    // 32-bit float formats
    R32F,     // 1 channel, 32-bit float
    RG32F,    // 2 channel, 32-bit float
    RGB32F,   // 3 channel, 32-bit float
    RGBA32F,  // 4 channel, 32-bit float

    // Depth
    DEPTH16,
    DEPTH24,
    DEPTH32,
    DEPTH32F,

    // Stencil
    STENCIL8,

    // Deapth-Stencil
    DEPTH24STENCIL8,
    DEPTH24FSTENCIL8F,
};

class Texture : public ImageBase
{
    friend class Device;

public:
    ~Texture() = default;

    virtual void PostCreate(ImagePostCreation& settings) override
    {
        width = settings.width;
        height = settings.height;
        channels = settings.channels;
        depth = settings.depth;
        format = GetTextureFormat(settings.channels, settings.depth);
    }

    TextureFormat Format() const
    {
        return format;
    }

    static TextureFormat GetTextureFormat(uint16_t channels, uint16_t depth)
    {
        if (depth <= 1)  // 8-bit unsigned integer formats
        {
            switch (channels)
            {
            case 1:
                return TextureFormat::R8;
            case 2:
                return TextureFormat::RG8;
            case 3:
                return TextureFormat::RGB8;
            case 4:
                return TextureFormat::RGBA8;
            }
        }
        else if (depth == 2)  // 16-bit float formats
        {
            switch (channels)
            {
            case 1:
                return TextureFormat::R16F;
            case 2:
                return TextureFormat::RG16F;
            case 3:
                return TextureFormat::RGB16F;
            case 4:
                return TextureFormat::RGBA16F;
            }
        }
        else if (depth == 4)  // 32-bit float formats
        {
            switch (channels)
            {
            case 1:
                return TextureFormat::R32F;
            case 2:
                return TextureFormat::RG32F;
            case 3:
                return TextureFormat::RGB32F;
            case 4:
                return TextureFormat::RGBA32F;
            }
        }
        FU_CORE_ASSERT(false, "Invalid texture format: unsupported channels or depth");
        return TextureFormat::RGBA8;
    }

protected:
    TextureFormat format;

    Texture(std::string_view name, std::string_view ext, TextureFormat format, uint32_t width, uint32_t height, uint32_t layers)
        : ImageBase(name, ext, width, height, format_to_channels(format), format_to_depth(format), layers)
        , format(format)
    {
    }
    Texture(std::string_view name, std::string_view ext, uint32_t layers)
        : ImageBase(name, ext, 0, 0, 0, 0, layers)
        , format(TextureFormat::RGB8)
    {
    }

    inline uint32_t calculate_mipmap_level(uint32_t width, uint32_t height)
    {
        return 1 + static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))));
    }

    uint32_t format_to_channels(TextureFormat format) const
    {
        switch (format)
        {
        // Color formats
        case TextureFormat::R8:
        case TextureFormat::R16F:
        case TextureFormat::R32F:
            return 1;

        case TextureFormat::RG8:
        case TextureFormat::RG16F:
        case TextureFormat::RG32F:
            return 2;

        case TextureFormat::RGB8:
        case TextureFormat::RGB16F:
        case TextureFormat::RGB32F:
            return 3;

        case TextureFormat::RGBA8:
        case TextureFormat::RGBA16F:
        case TextureFormat::RGBA32F:
            return 4;

        // Depth formats
        case TextureFormat::DEPTH16:
        case TextureFormat::DEPTH24:
        case TextureFormat::DEPTH32:
        case TextureFormat::DEPTH32F:
            return 1;

        // Stencil
        case TextureFormat::STENCIL8:
            return 1;

        // Depth + Stencil
        case TextureFormat::DEPTH24STENCIL8:
        case TextureFormat::DEPTH24FSTENCIL8F:
            return 2;
        }

        FU_CORE_ASSERT(false, "Unknown format in format_to_channels");
        return 1;
    }

    uint32_t format_to_depth(TextureFormat format) const
    {
        switch (format)
        {
        // Unsigned normalized
        case TextureFormat::R8:
        case TextureFormat::RG8:
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:
        case TextureFormat::STENCIL8:
            return 1;

        // 16-bit float or 16-bit depth
        case TextureFormat::R16F:
        case TextureFormat::RG16F:
        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F:
        case TextureFormat::DEPTH16:
            return 2;

        // 24-bit depth (stores in 3 bytes, aligns for 4)
        case TextureFormat::DEPTH24:
        case TextureFormat::DEPTH24STENCIL8:
        case TextureFormat::DEPTH24FSTENCIL8F:
            return 3;

        // 32-bit float or 32-bit depth
        case TextureFormat::R32F:
        case TextureFormat::RG32F:
        case TextureFormat::RGB32F:
        case TextureFormat::RGBA32F:
        case TextureFormat::DEPTH32:
        case TextureFormat::DEPTH32F:
            return 4;
        }

        FU_CORE_ASSERT(false, "Unknown format in format_to_depth");
        return 1;
    };
};
}  // namespace Fuego::Graphics
