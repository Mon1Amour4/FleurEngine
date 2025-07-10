#pragma once

#include <cstdint>

#include "Texture.h"

namespace Fuego::Graphics::TextureUtils
{

inline uint32_t CalculateMipmapLevels(uint32_t width, uint32_t height)
{
    return 1 + static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))));
}

inline uint32_t GetChannels(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8:
    case TextureFormat::R16F:
    case TextureFormat::R32F:
        return 1;
    case TextureFormat::RG8:
    case TextureFormat::RG16F:
        return 2;
    case TextureFormat::RGB8:
    case TextureFormat::RGB16F:
        return 3;
    case TextureFormat::RGBA8:
    case TextureFormat::RGBA16F:
    case TextureFormat::RGBA32F:
        return 4;
    default:
        throw std::runtime_error("Unknown texture format");
    }
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

    return TextureFormat::NONE;  // fallback if unsupported
}

}  // namespace Fuego::Graphics::TextureUtils