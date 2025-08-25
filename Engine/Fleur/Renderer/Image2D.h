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
        return m_Name;
    }
    inline std::string_view Ext() const
    {
        return m_Extension;
    }

    virtual uint32_t Width() const
    {
        return m_Width;
    }
    virtual uint32_t Height() const
    {
        return m_Height;
    }

    uint16_t Channels() const
    {
        return m_Channels;
    }
    uint32_t Depth() const
    {
        return m_Depth;
    }

    uint32_t Layers() const
    {
        return m_Layers;
    }

    uint32_t SizeBytes() const
    {
        return m_SizeBytes;
    }

    bool IsValid() const
    {
        return m_IsCreated;
    }

    virtual void PostCreate(ImagePostCreation& settings)
    {
        m_Width = settings.width;
        m_Height = settings.height;
        m_Channels = settings.channels;
        m_Depth = settings.depth;
    }

protected:
    ImageBase()
        : m_Name()
        , m_Width(0)
        , m_Height(0)
        , m_Channels(0)
        , m_Depth(0)
        , m_Layers(1)
        , m_SizeBytes(0)
        , m_IsCreated(false) {};
    ImageBase(std::string_view name, std::string_view ext, uint32_t layers);
    ImageBase(std::string_view name, std::string_view ext, uint32_t width, uint32_t height, uint16_t channels, uint16_t depth, uint32_t layers);

    std::string m_Name;
    std::string m_Extension;

    uint32_t m_Width;
    uint32_t m_Height;

    uint16_t m_Channels;
    uint16_t m_Depth;

    uint32_t m_Layers;

    uint32_t m_SizeBytes;

    bool m_IsCreated;
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
    Bitmap<BitmapFormat_UnsignedByte> m_Bitmap;

    Image2D(std::string_view name, std::string_view ext, Bitmap<BitmapFormat_UnsignedByte>&& IN bitmap, int w, int h, uint16_t channels, uint16_t depth);
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
    std::array<Image2D, 6> m_Faces;
};

}  // namespace Fleur::Graphics
