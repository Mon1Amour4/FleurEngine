#pragma once
#include <flpch.h>

#include "../../../CoreLib/Bitmap.hpp"

namespace Fleur::Graphics
{
class Color;
class CubemapImage;

struct ImagePostCreation
{
    uint32_t width;
    uint32_t height;
    uint16_t channels;
    uint32_t depth;
    const void* data;
};

class ImageBase
{
public:
    virtual ~ImageBase() = default;

    ImageBase(const ImageBase&) = delete;
    ImageBase& operator=(const ImageBase&) = delete;

    ImageBase(ImageBase&& other) noexcept;
    ImageBase& operator=(ImageBase&& other) noexcept;

    std::string_view Name() const
    {
        return name;
    }
    inline std::string_view Ext() const
    {
        return extension;
    }

    virtual uint32_t Width() const
    {
        return width;
    }
    virtual uint32_t Height() const
    {
        return height;
    }

    uint16_t Channels() const
    {
        return channels;
    }
    uint32_t Depth() const
    {
        return depth;
    }

    uint32_t Layers() const
    {
        return layers;
    }

    uint32_t SizeBytes() const
    {
        return size_bytes;
    }

    bool IsValid() const
    {
        return is_created;
    }

    virtual void PostCreate(ImagePostCreation& settings)
    {
        width = settings.width;
        height = settings.height;
        channels = settings.channels;
        depth = settings.depth;
    }

protected:
    ImageBase()
        : name()
        , width(0)
        , height(0)
        , channels(0)
        , depth(0)
        , layers(1)
        , size_bytes(0)
        , is_created(false) {};
    ImageBase(std::string_view name, std::string_view ext, uint32_t layers);
    ImageBase(std::string_view name, std::string_view ext, uint32_t width, uint32_t height, uint16_t channels, uint16_t depth, uint32_t layers);

    std::string name;
    std::string extension;

    uint32_t width;
    uint32_t height;

    uint16_t channels;
    uint16_t depth;

    uint32_t layers;

    uint32_t size_bytes;

    bool is_created;
};

class Image2D : public ImageBase
{
public:
    Image2D();
    ~Image2D() = default;

    Image2D(std::string_view name, std::string_view ext);
    Image2D(std::string_view name, std::string_view ext, unsigned char* data, int w, int h, uint16_t channels, uint16_t depth);
    Image2D(std::string_view name, std::string_view ext, int w, int h, uint16_t channels, uint16_t depth);

    Image2D& operator=(const Image2D& other) = delete;
    Image2D(const Image2D& other) = delete;

    Image2D& operator=(Image2D&& other) noexcept;
    Image2D(Image2D&& other) noexcept;

    const void* Data() const;

    virtual void PostCreate(ImagePostCreation& settings) override;

    Image2D FromEquirectangularToCross() const;
    CubemapImage FromCrossToCubemap() const;

private:
    Bitmap<BitmapFormat_UnsignedByte> bitmap;

    Image2D(std::string_view name, std::string_view ext, Bitmap<BitmapFormat_UnsignedByte>&& in_bitmap, int w, int h, uint16_t channels, uint16_t depth);
};


class CubemapImage : public ImageBase
{
public:
    enum class Face
    {
        Right = 0,   // +X
        Left = 1,    // -X
        Top = 2,     // +Y
        Bottom = 3,  // -Y
        Front = 4,   // +Z
        Back = 5     // -Z
    };

    CubemapImage(std::string_view name, std::string_view ext, std::array<Image2D, 6>&& faces);
    CubemapImage(std::string_view name, std::string_view ext);

    CubemapImage& operator=(const Image2D& other) = delete;
    CubemapImage(const CubemapImage& other) = delete;

    CubemapImage& operator=(CubemapImage&& other) noexcept;
    CubemapImage(CubemapImage&& other) noexcept;

    const Fleur::Graphics::Image2D& GetFace(Face face) const;

    const Image2D* Data() const;

    virtual void PostCreate(ImagePostCreation& settings) override;

private:
    std::array<Image2D, 6> faces;
};

}  // namespace Fleur::Graphics
