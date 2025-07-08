#pragma once
#include <fupch.h>

#include "../../../CoreLib/Bitmap.hpp"

namespace Fuego::Graphics
{
class Color;
class CubemapImage;

class ImageBase
{
public:
    struct ImagePostCreateion
    {
        uint32_t width;
        uint32_t height;
        uint16_t channels;
        uint16_t depth;
        const void* data;
    };

    virtual ~ImageBase() = default;

    virtual void PostCreate(ImagePostCreateion& settings)
    {
        FU_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");
        width = settings.width;
        height = settings.height;
        depth = settings.depth;
        channels = settings.channels;
    }

    std::string_view Name() const
    {
        return name;
    }
    inline std::string_view Ext() const
    {
        return ext;
    }

    virtual const void* Data() const = 0;
    uint32_t SizeBytes() const
    {
        return size_bytes;
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
    uint16_t Depth() const
    {
        return depth;
    }

    bool IsValid() const
    {
        return is_created;
    }

protected:
    ImageBase()
        : name()
        , ext()
        , width(0)
        , height(0)
        , channels(0)
        , depth(0)
        , size_bytes(0)
        , is_created(false) {};
    ImageBase(std::string_view name, std::string_view ext)
        : name(name)
        , ext(ext)
        , width(0)
        , height(0)
        , channels(0)
        , depth(0)
        , size_bytes(0)
        , is_created(false) {};
    ImageBase(std::string_view name, std::string_view ext, uint32_t width, uint32_t height, uint16_t channels, uint16_t depth)
        : name(name)
        , ext(ext)
        , width(width)
        , height(height)
        , channels(channels)
        , depth(depth)
        , size_bytes(width * height * channels * depth)
        , is_created(false) {};


    std::string name;
    std::string ext;
    uint32_t width;
    uint32_t height;
    uint16_t channels;
    uint16_t depth;
    uint32_t size_bytes;
    bool is_created;
};

class Image2D : public ImageBase
{
public:
    Image2D()
        : ImageBase() {};
    Image2D(std::string_view name, std::string_view ext)
        : ImageBase(name, ext) {};
    Image2D(std::string_view name, std::string_view ext, unsigned char* data, int w, int h, uint16_t channels, uint16_t depth)
        : ImageBase(name, ext, w, h, channels, depth)
        , bitmap(w, h, channels)
    {
        FU_CORE_ASSERT(depth > 0 && channels > 0, "Invalid Image data");
        memcpy(bitmap.Data(), data, bitmap.GetSizeBytes());
        is_created = true;
    }

    ~Image2D() = default;


    Image2D& operator=(const Image2D& other) = delete;
    Image2D(const Image2D& other) = delete;

    Image2D& operator=(Image2D&& other) noexcept
    {
        if (this != &other)
        {
            bitmap = std::move(other.bitmap);
            width = other.width;
            height = other.height;
            is_created = other.is_created;
            name = std::move(other.name);
            ext = std::move(other.ext);
            channels = other.channels;
            depth = other.depth;

            other.width = 0;
            other.height = 0;
            other.is_created = false;
            other.channels = 0;
            other.depth = 0;
        }
        return *this;
    }
    Image2D(Image2D&& other) noexcept
    {
        bitmap = std::move(other.bitmap);
        width = other.width;
        height = other.height;
        is_created = other.is_created;
        name = std::move(other.name);
        ext = std::move(other.ext);
        channels = other.channels;
        depth = other.depth;

        other.width = 0;
        other.height = 0;
        other.is_created = false;
        other.channels = 0;
        other.depth = 0;
    }


    virtual const void* Data() const override
    {
        return bitmap.Data();
    }

    virtual void PostCreate(ImagePostCreateion& settings) override
    {
        ImageBase::PostCreate(settings);
        bitmap = Bitmap<BitmapFormat_UnsignedByte>(settings.width, settings.height, settings.channels);
        memcpy_s(bitmap.Data(), bitmap.GetSizeBytes(), settings.data, settings.width * settings.height * settings.channels * settings.depth);
        is_created = true;
    }

    CubemapImage GenerateCubemapImage() const;

private:
    Image2D(std::string_view name, Bitmap<BitmapFormat_UnsignedByte>&& in_bitmap, int w, int h, uint16_t channels, uint16_t depth)
        : ImageBase(name, "-", w, h, channels, depth)
        , bitmap(std::move(in_bitmap))
    {
    }
    Bitmap<BitmapFormat_UnsignedByte> bitmap;
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

    CubemapImage(std::string_view name, std::array<Image2D, 6>&& faces);


    // Cross Layout:
    //          +------+
    //          |  +Y  |
    //  +-------+------+-------+------+
    //  |  -X   |  +Z  |  +X   |  -Z  |
    //  +-------+------+-------+------+
    //          |  -Y  |
    //          |      |
    // static CubemapImage FromCrossLayout(const Image2D& layout);

    const Image2D& GetFace(Face face) const;

    virtual const void* Data() const override
    {
        return reinterpret_cast<const void*>(faces[0].Data());
    }

    // std::shared_ptr<Image2D> AssembledCrossLayout() const;
    // void SaveToDisk(std::string_view directory) const;

private:
    std::array<Image2D, 6> faces;
};

}  // namespace Fuego::Graphics
