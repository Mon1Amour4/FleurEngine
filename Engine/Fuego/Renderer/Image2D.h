#pragma once
#include <fupch.h>

#include "../../../CoreLib/Bitmap.hpp"

namespace Fuego::Graphics
{
class Color;
class CubemapImage;
class Image2D
{
public:
    Image2D(std::string_view name, std::string_view ext, unsigned char* data, int w, int h, uint16_t channels, uint16_t depth);
    Image2D(std::string_view name, std::string_view ext);
    Image2D()
        : width(0)
        , height(0)
        , is_created(false)
        , channels(0)
        , depth(0)
    {
    }
    ~Image2D() = default;

    struct Image2DPostCreateion
    {
        uint32_t width;
        uint32_t height;
        uint16_t channels;
        uint16_t depth;
        unsigned char* data;
    };

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

    inline uint32_t Width() const
    {
        return width;
    }
    inline uint32_t Height() const
    {
        return height;
    }
    inline uint16_t Depth() const
    {
        return depth;
    }
    inline uint16_t Channels() const
    {
        return channels;
    }
    const unsigned char* Data() const
    {
        return reinterpret_cast<const unsigned char*>(bitmap.Data());
    }
    inline std::string_view Name() const
    {
        return name;
    }
    inline std::string_view Ext() const
    {
        return ext;
    }
    uint32_t SizeBytes() const
    {
        return bitmap.GetSizeBytes();
    }

    void PostCreate(Image2DPostCreateion& settings);
    inline bool IsValid() const
    {
        return is_created;
    }

    CubemapImage GenerateCubemapImage() const;

private:
    Image2D(std::string_view name, Bitmap<BitmapFormat_UnsignedByte>&& data, int face_size)
        : name(name)
        , bitmap(std::move(data))
        , width(face_size)
        , height(face_size)
        , is_created(true)
        , ext("")
        , channels(3)
        , depth(1)
    {
    }
    Bitmap<BitmapFormat_UnsignedByte> bitmap;
    uint32_t width;
    uint32_t height;
    bool is_created;
    std::string name;
    std::string ext;
    uint16_t channels;
    uint16_t depth;
};

class CubemapImage
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
    const std::array<Image2D, 6>& Faces() const;

    uint32_t FaceSize() const;
    // uint16_t Channels() const;
    // uint16_t Depth() const;
    // uint32_t SizeBytes() const;
    std::string_view Name() const;

    // std::shared_ptr<Image2D> AssembledCrossLayout() const;
    // void SaveToDisk(std::string_view directory) const;

private:
    std::string name;
    std::array<Image2D, 6> faces;
    uint32_t face_size;
};

}  // namespace Fuego::Graphics
