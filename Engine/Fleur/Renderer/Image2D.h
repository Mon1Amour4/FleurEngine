#pragma once
#include <flpch.h>

#include "Bitmap.hpp"
#include "Renderer/RenderViews.hpp"

#define CUBEMAP_LAYERS_COUNT 6

namespace Fleur::Graphics
{
class Color;
class CubemapImage;

struct ImagePostCreation
{
    uint32_t width{};
    uint32_t height{};
    uint16_t channels{};
    uint16_t depthBytes{};
    const void* data{};
};

class ImageBase
{
public:
    virtual ~ImageBase() = default;

    ImageBase(const ImageBase&) = delete;
    ImageBase& operator=(const ImageBase&) = delete;

    ImageBase(ImageBase&& other) noexcept;
    ImageBase& operator=(ImageBase&& other) noexcept;

    std::string_view GetName() const
    {
        return m_Name;
    }
    virtual uint32_t GetWidth() const
    {
        return m_Width;
    }
    virtual uint32_t GetHeight() const
    {
        return m_Height;
    }

    uint16_t GetChannelsCount() const
    {
        return m_Channels;
    }
    uint16_t GetDepth() const
    {
        return m_DepthBytes;
    }

    uint16_t GetLayersCount() const
    {
        return m_Layers;
    }

    [[nodiscard]] size_t GetSizeBytes() const
    {
        assert(m_SizeBytes > 0);
        return m_SizeBytes;
    }

    [[nodiscard]] bool IsValid() const
    {
        return m_IsCreated;
    }

    virtual void PostCreate(ImagePostCreation& settings)
    {
        m_Width = settings.width;
        m_Height = settings.height;
        m_Channels = settings.channels;
        m_DepthBytes = settings.depthBytes;
        m_SizeBytes = m_Width * m_Height * m_Channels * m_DepthBytes;
    }

protected:
    ImageBase()
        : m_Name()
        , m_Width(0)
        , m_Height(0)
        , m_Channels(0)
        , m_DepthBytes(0)
        , m_Layers(1)
        , m_SizeBytes(0)
        , m_IsCreated(false) {};
    ImageBase(std::string_view name, uint16_t layers);
    ImageBase(std::string_view name, uint32_t width, uint32_t height, uint16_t channels, uint16_t depth, uint16_t layers);

    std::string m_Name;

    uint32_t m_Width, m_Height;

    uint16_t m_Channels, m_DepthBytes, m_Layers;

    size_t m_SizeBytes;

    bool m_IsCreated;
};

class Image2D : public ImageBase
{
public:
    Image2D();
    ~Image2D() = default;

    Image2D(std::string_view name);
    Image2D(std::string_view name, unsigned char* data, int w, int h, uint16_t channels, uint16_t depth);
    Image2D(std::string_view name, int w, int h, uint16_t channels, uint16_t depth);

    Image2D& operator=(const Image2D& other) = delete;
    Image2D(const Image2D& other) = delete;

    Image2D& operator=(Image2D&& other) noexcept;
    Image2D(Image2D&& other) noexcept;

    inline const void* GetData() const
    {
        return m_Bitmap.GetData();
    }

    virtual void PostCreate(ImagePostCreation& settings) override;

    Fleur::Graphics::SFLImageView GetView() const;

    inline const Bitmap<BitmapFormat_UnsignedByte>* GetBitmap() const
    {
        return &m_Bitmap;
    }

private:
    Bitmap<BitmapFormat_UnsignedByte> m_Bitmap;

    Image2D(std::string_view name, Bitmap<BitmapFormat_UnsignedByte>&& bitmap, int w, int h, uint16_t channels, uint16_t depth);
};


class CubemapImage : public ImageBase
{
public:
    enum class EFace
    {
        Right = 0,   // +X
        Left = 1,    // -X
        Top = 2,     // +Y
        Bottom = 3,  // -Y
        Front = 4,   // +Z
        Back = 5     // -Z
    };

    CubemapImage() = default;
    CubemapImage(std::string_view name);
    static CubemapImage FromEquirectangular(const Image2D& src);
    static CubemapImage FromCross(const Image2D& src);
    static CubemapImage FromFaces(const std::array<Image2D, 6>& faces);

    CubemapImage& operator=(const Image2D& other) = delete;
    CubemapImage(const CubemapImage& other) = delete;

    CubemapImage& operator=(CubemapImage&& other) noexcept;
    CubemapImage(CubemapImage&& other) noexcept;

    inline const unsigned char* GetData() const
    {
        return (const unsigned char*)m_Data.data();
    }
    inline const unsigned char* GetFaceData(uint32_t face) const
    {
        if (face > CUBEMAP_LAYERS_COUNT - 1)
            return nullptr;

        return (unsigned char*)m_Data.data() + (m_Width * m_Height * face);
    }
    inline unsigned char* GetFaceData(uint32_t face)
    {
        if (face > CUBEMAP_LAYERS_COUNT - 1)
            return nullptr;

        return (unsigned char*)m_Data.data() + (m_Width * m_Height * m_Channels * m_DepthBytes * face);
    }
    virtual void PostCreate(ImagePostCreation& settings) override;

    Fleur::Graphics::SFLImageView GetView() const;

private:
    std::vector<std::byte> m_Data;
};

}  // namespace Fleur::Graphics
