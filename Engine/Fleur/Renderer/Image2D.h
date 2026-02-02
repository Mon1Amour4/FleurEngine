#pragma once
#include <flpch.h>

#include "Bitmap.hpp"
#include "Renderer/RenderViews.hpp"

namespace Fleur::Graphics
{
class Color;
class CubemapImage;

struct ImagePostCreation
{
    uint32_t width;
    uint32_t height;
    uint16_t channels;
    uint16_t depth;
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
        return m_Depth;
    }

    uint16_t GetLayersCount() const
    {
        return m_Layers;
    }

    [[nodiscard]] size_t GetSizeBytes() const
    {
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
    ImageBase(std::string_view name, uint16_t layers);
    ImageBase(std::string_view name, uint32_t width, uint32_t height, uint16_t channels, uint16_t depth, uint16_t layers);

    std::string m_Name;

    uint32_t m_Width, m_Height;

    uint16_t m_Channels, m_Depth, m_Layers;

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

    const void* Data() const;

    virtual void PostCreate(ImagePostCreation& settings) override;

    Image2D FromEquirectangularToCross() const;
    CubemapImage FromCrossToCubemap() const;

    Fleur::Graphics::SFLImageView GetView() const;

private:
    Bitmap<BitmapFormat_UnsignedByte> m_Bitmap;

    Image2D(std::string_view name, Bitmap<BitmapFormat_UnsignedByte>&& IN bitmap, int w, int h, uint16_t channels, uint16_t depth);
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
    CubemapImage(std::string_view name, std::array<Image2D, 6>&& faces);

    CubemapImage& operator=(const Image2D& other) = delete;
    CubemapImage(const CubemapImage& other) = delete;

    CubemapImage& operator=(CubemapImage&& other) noexcept;
    CubemapImage(CubemapImage&& other) noexcept;

    const Fleur::Graphics::Image2D& GetFace(EFace face) const;

    const Image2D* GetData() const;

    virtual void PostCreate(ImagePostCreation& settings) override;

    Fleur::Graphics::SFLImageView GetView() const;

private:
    std::array<Image2D, 6> m_Faces;
};

}  // namespace Fleur::Graphics
