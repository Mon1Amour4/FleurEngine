#pragma once

#include "Color.h"
#include "Image2D.h"

namespace Fleur::Graphics
{

struct CubemapInitData
{
    const std::shared_ptr<Fleur::Graphics::Image2D> right;
    const std::shared_ptr<Fleur::Graphics::Image2D> left;
    const std::shared_ptr<Fleur::Graphics::Image2D> top;
    const std::shared_ptr<Fleur::Graphics::Image2D> bottom;
    const std::shared_ptr<Fleur::Graphics::Image2D> back;
    const std::shared_ptr<Fleur::Graphics::Image2D> front;
};

class Device;

enum class ETextureFormat
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
        m_Width = settings.width;
        m_Height = settings.height;
        m_Channels = settings.channels;
        m_DepthBytes = settings.depthBytes;
        m_Format = GetTextureFormat(settings.channels, settings.depthBytes);
    }

    ETextureFormat Format() const
    {
        return m_Format;
    }

    static ETextureFormat GetTextureFormat(uint16_t channels, uint16_t depth)
    {
        if (depth <= 1)  // 8-bit unsigned integer formats
        {
            switch (channels)
            {
            case 1:
                return ETextureFormat::R8;
            case 2:
                return ETextureFormat::RG8;
            case 3:
                return ETextureFormat::RGB8;
            case 4:
                return ETextureFormat::RGBA8;
            }
        }
        else if (depth == 2)  // 16-bit float formats
        {
            switch (channels)
            {
            case 1:
                return ETextureFormat::R16F;
            case 2:
                return ETextureFormat::RG16F;
            case 3:
                return ETextureFormat::RGB16F;
            case 4:
                return ETextureFormat::RGBA16F;
            }
        }
        else if (depth == 4)  // 32-bit float formats
        {
            switch (channels)
            {
            case 1:
                return ETextureFormat::R32F;
            case 2:
                return ETextureFormat::RG32F;
            case 3:
                return ETextureFormat::RGB32F;
            case 4:
                return ETextureFormat::RGBA32F;
            }
        }
        FL_CORE_ASSERT(false, "Invalid texture format: unsupported channels or depth");
        return ETextureFormat::RGBA8;
    }

protected:
    ETextureFormat m_Format;

    Texture(std::string_view name, std::string_view ext, ETextureFormat format, uint32_t width, uint32_t height, uint16_t layers)
        : ImageBase(name, width, height, FormatToChannels(format), FormatToDepth(format), layers)
        , m_Format(format)
    {
    }
    Texture(std::string_view name, std::string_view ext, uint16_t layers)
        : ImageBase(name, 0, 0, 0, 0, layers)
        , m_Format(ETextureFormat::RGB8)
    {
    }

    inline uint32_t calculate_mipmap_level(uint32_t width, uint32_t height)
    {
        return 1 + static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))));
    }

    uint16_t FormatToChannels(ETextureFormat format) const
    {
        switch (format)
        {
        // Color formats
        case ETextureFormat::R8:
        case ETextureFormat::R16F:
        case ETextureFormat::R32F:
            return 1;

        case ETextureFormat::RG8:
        case ETextureFormat::RG16F:
        case ETextureFormat::RG32F:
            return 2;

        case ETextureFormat::RGB8:
        case ETextureFormat::RGB16F:
        case ETextureFormat::RGB32F:
            return 3;

        case ETextureFormat::RGBA8:
        case ETextureFormat::RGBA16F:
        case ETextureFormat::RGBA32F:
            return 4;

        // Depth formats
        case ETextureFormat::DEPTH16:
        case ETextureFormat::DEPTH24:
        case ETextureFormat::DEPTH32:
        case ETextureFormat::DEPTH32F:
            return 1;

        // Stencil
        case ETextureFormat::STENCIL8:
            return 1;

        // Depth + Stencil
        case ETextureFormat::DEPTH24STENCIL8:
        case ETextureFormat::DEPTH24FSTENCIL8F:
            return 2;
        }

        FL_CORE_ASSERT(false, "Unknown format in format_to_channels");
        return 1;
    }

    uint16_t FormatToDepth(ETextureFormat format) const
    {
        switch (format)
        {
        // Unsigned normalized
        case ETextureFormat::R8:
        case ETextureFormat::RG8:
        case ETextureFormat::RGB8:
        case ETextureFormat::RGBA8:
        case ETextureFormat::STENCIL8:
            return 1;

        // 16-bit float or 16-bit depth
        case ETextureFormat::R16F:
        case ETextureFormat::RG16F:
        case ETextureFormat::RGB16F:
        case ETextureFormat::RGBA16F:
        case ETextureFormat::DEPTH16:
            return 2;

        // 24-bit depth (stores in 3 bytes, aligns for 4)
        case ETextureFormat::DEPTH24:
        case ETextureFormat::DEPTH24STENCIL8:
        case ETextureFormat::DEPTH24FSTENCIL8F:
            return 3;

        // 32-bit float or 32-bit depth
        case ETextureFormat::R32F:
        case ETextureFormat::RG32F:
        case ETextureFormat::RGB32F:
        case ETextureFormat::RGBA32F:
        case ETextureFormat::DEPTH32:
        case ETextureFormat::DEPTH32F:
            return 4;
        }

        FL_CORE_ASSERT(false, "Unknown format in format_to_depth");
        return 1;
    };
};
}  // namespace Fleur::Graphics
