#pragma once

#include "Color.h"
#include "Image2D.h"
#include "TextureUtills.h"

namespace Fuego::Graphics
{

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

    NONE
};

class TextureBase
{
public:
    virtual ~TextureBase() = default;
    virtual void PostCreate(std::shared_ptr<Fuego::Graphics::Image2D> img)
    {
        const auto& image = *img.get();
        width = image.Width();
        height = image.Height();
        format = GetTextureFormat(image.Channels(), image.Depth());
        is_created = true;
    }

    virtual bool IsValid() const = 0;

    virtual inline uint32_t Width() const
    {
        return width;
    }
    virtual inline uint32_t Height() const
    {
        return height;
    }

    TextureFormat Format() const
    {
        return format;
    }

    virtual inline std::string_view Name() const
    {
        return name;
    }

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
        }
    }

    inline TextureFormat GetTextureFormat(uint16_t channels, uint16_t depth)
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

    }  // namespace Fuego::Graphics::TextureUtils
protected:
    TextureBase(std::string_view name, TextureFormat format, int width, int height)
        : name(name)
        , format(format)
        , width(width)
        , height(height)
        , is_created(false) {};

    TextureBase(std::string_view name)
        : name(name)
        , format(TextureFormat::NONE)
        , width(0)
        , height(0)
        , is_created(false)
    {
    }

    bool is_created;
    std::string name;
    uint32_t width;
    uint32_t height;
    TextureFormat format;

};  // namespace Fuego::Graphics

class Texture2D : public TextureBase
{
public:
    virtual ~Texture2D() = default;

    virtual void PostCreate(std::shared_ptr<Fuego::Graphics::Image2D> img) override
    {
        TextureBase::PostCreate(img);
    }

    virtual bool IsValid() const override
    {
        return is_created;
    }

protected:
    Texture2D(std::string_view name, TextureFormat format, int width, int height)
        : TextureBase(name, format, width, height)
    {
        int a = 5;
    }
    Texture2D(std::string_view name)
        : TextureBase(name) {};
};

class TextureCubemap : public TextureBase
{
public:
    virtual ~TextureCubemap() = default;

    virtual void PostCreate(std::shared_ptr<Fuego::Graphics::Image2D> img) override
    {
        TextureBase::PostCreate(img);
    }
    virtual bool IsValid() const override
    {
        return is_created;
    }

protected:
    TextureCubemap(std::string_view name, TextureFormat format, int width, int height)
        : TextureBase(name, format, width, height) {};
    TextureCubemap(std::string_view name)
        : TextureBase(name) {};
};

class TextureView
{
public:
    virtual ~TextureView() = default;
};

}  // namespace Fuego::Graphics
