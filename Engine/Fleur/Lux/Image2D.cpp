#include "Image2D.h"

#include "Color.h"
#include "FleurAllocator.hpp"
#include "Services/ServiceLocator.h"


// ---------- ImageBase ----------
Fleur::Graphics::ImageBase::ImageBase(std::string_view name, uint16_t layers)
    : m_Name(name)
    , m_Width(0)
    , m_Height(0)
    , m_Channels(0)
    , m_DepthBytes(0)
    , m_Layers(layers)
    , m_SizeBytes(0)
    , m_IsCreated(false)
{
}
Fleur::Graphics::ImageBase::ImageBase(std::string_view name, uint32_t width, uint32_t height, uint16_t channels, uint16_t depth, uint16_t layers)
    : m_Name(name)
    , m_Width(width)
    , m_Height(height)
    , m_Channels(channels)
    , m_DepthBytes(depth)
    , m_Layers(layers)
    , m_SizeBytes(width * height * channels * depth * layers)
    , m_IsCreated(false)
{
}

Fleur::Graphics::ImageBase::ImageBase(ImageBase&& other) noexcept
    : m_Name(std::move(other.m_Name))
    , m_Width(other.m_Width)
    , m_Height(other.m_Height)
    , m_Channels(other.m_Channels)
    , m_DepthBytes(other.m_DepthBytes)
    , m_Layers(other.m_Layers)
    , m_SizeBytes(other.m_SizeBytes)
    , m_IsCreated(other.m_IsCreated)
{
    other.m_Width = 0;
    other.m_Height = 0;
    other.m_Channels = 0;
    other.m_DepthBytes = 0;
    other.m_Layers = 0;
    other.m_SizeBytes = 0;
    other.m_IsCreated = false;
}
Fleur::Graphics::ImageBase& Fleur::Graphics::ImageBase::operator=(ImageBase&& other) noexcept
{
    if (this != &other)
    {
        m_Name = std::move(other.m_Name);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_Channels = other.m_Channels;
        m_DepthBytes = other.m_DepthBytes;
        m_Layers = other.m_Layers;
        m_SizeBytes = other.m_SizeBytes;
        m_IsCreated = other.m_IsCreated;

        other.m_Width = 0;
        other.m_Height = 0;
        other.m_Channels = 0;
        other.m_DepthBytes = 0;
        other.m_Layers = 0;
        other.m_SizeBytes = 0;
        other.m_IsCreated = false;
    }
    return *this;
}


// ---------- Image2D ----------
Fleur::Graphics::Image2D::Image2D()
    : ImageBase()
{
}
Fleur::Graphics::Image2D::Image2D(std::string_view name)
    : ImageBase(name, 1)
{
}
Fleur::Graphics::Image2D::Image2D(std::string_view name, int w, int h, uint16_t channels, uint16_t depth)
    : ImageBase(name, w, h, channels, depth, 1)
{
}
Fleur::Graphics::Image2D::Image2D(std::string_view name, unsigned char* data, int w, int h, uint16_t channels, uint16_t depth)
    : ImageBase(name, w, h, channels, depth, 1)
    , m_Bitmap(w, h, channels)
{
    FL_CORE_ASSERT(depth > 0 && channels > 0, "Invalid Image data");
    memcpy(m_Bitmap.GetData(), data, m_Bitmap.GetSizeBytes());
    m_IsCreated = true;
}
Fleur::Graphics::Image2D::Image2D(std::string_view name, Bitmap<BitmapFormat_UnsignedByte>&& IN inBitmap, int w, int h, uint16_t channels, uint16_t depth)
    : ImageBase(name, w, h, channels, depth, 1)
    , m_Bitmap(std::move(inBitmap))
{
    m_IsCreated = true;
};

Fleur::Graphics::Image2D& Fleur::Graphics::Image2D::operator=(Fleur::Graphics::Image2D&& other) noexcept
{
    if (this != &other)
    {
        m_Bitmap = std::move(other.m_Bitmap);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_IsCreated = other.m_IsCreated;
        m_Name = std::move(other.m_Name);
        m_Channels = other.m_Channels;
        m_DepthBytes = other.m_DepthBytes;
        m_Layers = other.m_Layers;
        m_SizeBytes = other.m_SizeBytes;

        other.m_Width = 0;
        other.m_Height = 0;
        other.m_IsCreated = false;
        other.m_Channels = 0;
        other.m_DepthBytes = 0;
        other.m_Layers = 0;
        other.m_SizeBytes = 0;
    }
    return *this;
}
Fleur::Graphics::Image2D::Image2D(Fleur::Graphics::Image2D&& other) noexcept
{
    m_Bitmap = std::move(other.m_Bitmap);
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_IsCreated = other.m_IsCreated;
    m_Name = std::move(other.m_Name);
    m_Channels = other.m_Channels;
    m_DepthBytes = other.m_DepthBytes;
    m_Layers = other.m_Layers;
    m_SizeBytes = other.m_SizeBytes;

    other.m_Width = 0;
    other.m_Height = 0;
    other.m_IsCreated = false;
    other.m_Channels = 0;
    other.m_DepthBytes = 0;
    other.m_Layers = 0;
    other.m_SizeBytes = 0;
}

Fleur::Graphics::SFLImageView Fleur::Graphics::Image2D::GetView() const
{
    return {0, (const char*)m_Bitmap.GetData(), m_Width, m_Height, m_Layers, m_Channels};
}
void Fleur::Graphics::Image2D::PostCreate(ImagePostCreation& settings)
{
    FL_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");

    ImageBase::PostCreate(settings);

    m_Bitmap = Bitmap<BitmapFormat_UnsignedByte>(settings.width, settings.height, settings.channels);
    memcpy_s(m_Bitmap.GetData(), m_Bitmap.GetSizeBytes(), settings.data, settings.width * settings.height * settings.channels * settings.depthBytes);

    m_IsCreated = true;
}


// ---------- CubemapImage ----------
Fleur::Graphics::CubemapImage::CubemapImage(std::string_view name)
    : ImageBase(name, 6)
{
}
Fleur::Graphics::CubemapImage& Fleur::Graphics::CubemapImage::operator=(CubemapImage&& other) noexcept
{
    if (this != &other)
    {
        ImageBase::operator=(std::move(other));
        m_Data = std::move(other.m_Data);
    }
    return *this;
}
Fleur::Graphics::CubemapImage::CubemapImage(CubemapImage&& other) noexcept
    : ImageBase(std::move(other))
    , m_Data(std::move(other.m_Data))
{
}

Fleur::Graphics::CubemapImage Fleur::Graphics::CubemapImage::FromFaces(const std::array<Image2D, 6>& faces)
{
    Fleur::Graphics::CubemapImage cubemap{};
    cubemap.m_Name = faces[0].GetName();
    cubemap.m_Width = faces[0].GetWidth();
    cubemap.m_Height = faces[0].GetHeight();
    assert(cubemap.m_Width == cubemap.m_Height);
    cubemap.m_Channels = faces[0].GetChannelsCount();
    cubemap.m_DepthBytes = faces[0].GetDepth();
    cubemap.m_Layers = CUBEMAP_LAYERS_COUNT;

    size_t layerSize = faces[0].GetSizeBytes();
    cubemap.m_Data.resize(layerSize * CUBEMAP_LAYERS_COUNT);
    for (size_t i = 0; i < CUBEMAP_LAYERS_COUNT; i++)
    {
        memcpy(static_cast<void*>(cubemap.GetFaceData(i)), faces[i].GetData(), faces[i].GetSizeBytes());
    }
    return cubemap;
}
Fleur::Graphics::CubemapImage Fleur::Graphics::CubemapImage::FromEquirectangular(const Image2D& src)
{
    // 1. Stage one: From Equirectangular to cross:
    uint32_t faceSize = src.GetWidth() / 4;
    constexpr float pi = Fleur::Math::pi<float>();
    const Fleur::Bitmap<BitmapFormat_UnsignedByte>* sourceBitmap = src.GetBitmap();
    Bitmap<BitmapFormat_UnsignedByte> outBitmap(faceSize * 4, faceSize * 3, src.GetChannelsCount());

    Fleur::Graphics::CubemapImage cubemap{};
    cubemap.m_Name = src.GetName();
    cubemap.m_Width = faceSize;
    cubemap.m_Height = faceSize;
    cubemap.m_Channels = src.GetChannelsCount();
    cubemap.m_DepthBytes = src.GetDepth();
    cubemap.m_Layers = CUBEMAP_LAYERS_COUNT;

    size_t layerSize = faceSize * faceSize * cubemap.m_Channels * cubemap.m_DepthBytes;

    struct FacePos
    {
        int x, y;
    };
    const FacePos facePos[12] = {
        {},          // 0
        {1, 0},      // 1 POSITIVE_Y
        {},     {},  // 2,3
        {0, 1},      // 4 NEGATIVE_X
        {3, 1},      // 5 NEGATIVE_Z
        {2, 1},      // 6 POSITIVE_X
        {1, 1},      // 7 POSITIVE_Z
        {},          // 8
        {1, 2},      // 9 NEGATIVE_Y
        {},     {}   // 10,11
    };

    std::vector<float, Fleur::Memory::FleurAllocator<float>> normU(faceSize);
    std::vector<float, Fleur::Memory::FleurAllocator<float>> normV(faceSize);
    for (uint32_t i = 0; i < faceSize; ++i)
    {
        normU[i] = ((i + 0.5f) / faceSize) * 2.f - 1.f;
        normV[i] = ((i + 0.5f) / faceSize) * 2.f - 1.f;
    }

    for (size_t face = 0; face < 12; face++)
    {
        if (face == 0 || face == 2 || face == 3 || face == 8 || face == 10 || face == 11)
        {
            continue;
        }

        const FacePos& fp = facePos[face];

        for (uint32_t coord_u = 0; coord_u < faceSize; coord_u++)
        {
            for (uint32_t coord_v = 0; coord_v < faceSize; coord_v++)
            {
                float u = normU[coord_u];
                float v = normV[coord_v];

                float dx, dy, dz;
                switch (face)
                {
                case 6:  // +X (right)
                    dx = 1.f;
                    dy = -v;
                    dz = u;
                    break;
                case 4:  // -X (left)
                    dx = -1.f;
                    dy = -v;
                    dz = -u;
                    break;
                case 1:  // +Y (top)
                    dx = u;
                    dy = 1.f;
                    dz = -v;
                    break;
                case 9:  // -Y (bottom)
                    dx = u;
                    dy = -1.f;
                    dz = v;
                    break;
                case 7:  // +Z (back)
                    dx = u;
                    dy = -v;
                    dz = -1.f;
                    break;
                case 5:  // -Z (front)
                    dx = -u;
                    dy = -v;
                    dz = 1.f;
                    break;
                }

                float sx = -dz;
                float sy = dx;
                float sz = dy;

                float radius = Fleur::Math::sqrt(sx * sx + sy * sy + sz * sz);
                float invRadius = 1.0f / radius;

                float phi = atan2(sy, sx);           // [-pi, +pi]
                float theta = acos(sz * invRadius);  // [0, pi]

                float uu = (phi + pi) / (2.f * pi);  // [0,1]
                float vv = theta / pi;               // [0,1]

                float x = static_cast<float>(uu * (src.GetWidth() - 1));
                float y = static_cast<float>(vv * (src.GetHeight() - 1));


                // bilinear interpolation:
                uint32_t leftX = static_cast<uint32_t>(std::floor(x));

                FL_CORE_ASSERT(leftX >= 0, "");

                uint32_t rightX = std::min(static_cast<uint32_t>(std::floor(leftX + 1)), src.GetWidth() - 1);

                uint32_t topY = static_cast<uint32_t>(std::floor(y));
                uint32_t bottomY = std::min(static_cast<uint32_t>(topY + 1), src.GetHeight() - 1);

                float shiftX = x - leftX;
                float shiftY = y - topY;

                // w00 -- w01
                // ----uv----
                // w10 -- w11
                float w00 = (1.f - shiftX) * (1.f - shiftY);
                float w01 = shiftX * (1.f - shiftY);
                float w10 = (1.f - shiftX) * shiftY;
                float w11 = shiftX * shiftY;

                Fleur::Math::vec4 c00 = sourceBitmap->GetPixel(leftX, topY);
                Fleur::Math::vec4 c01 = sourceBitmap->GetPixel(rightX, topY);
                Fleur::Math::vec4 c10 = sourceBitmap->GetPixel(leftX, bottomY);
                Fleur::Math::vec4 c11 = sourceBitmap->GetPixel(rightX, bottomY);

                Fleur::Math::vec4 color = Fleur::Math::vec4((c00 * w00 + c01 * w01 + c10 * w10 + c11 * w11));

                uint32_t outX = fp.x * faceSize + coord_u;
                uint32_t outY = fp.y * faceSize + coord_v;
                outBitmap.SetPixel(outX, outY, color);
            }
        }
    }

    // 2. Stage two: from equirectangular to cubemap
    cubemap.m_Data.resize(faceSize * faceSize * cubemap.m_Channels * CUBEMAP_LAYERS_COUNT);
    auto uploadFace = [&](uint32_t startX, uint32_t startY, uint32_t outFace)
    {
        Fleur::Bitmap<BitmapFormat_UnsignedByte> tmp(faceSize, faceSize, cubemap.m_Channels);
        for (uint32_t y = 0; y < faceSize; ++y)
        {
            for (uint32_t x = 0; x < faceSize; ++x)
            {
                Fleur::Math::vec4 color = outBitmap.GetPixel(startX + x, startY + y);
                tmp.SetPixel(x, y, color);
            }
        }
        memmove(cubemap.GetFaceData(outFace), tmp.GetData(), layerSize);
    };

    // Face indices: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
    uploadFace(faceSize * 2, faceSize, 0);  // +X (right)
    uploadFace(0, faceSize, 1);             // -X (left)
    uploadFace(faceSize, 0, 2);             // +Y (top)
    uploadFace(faceSize, faceSize * 2, 3);  // -Y (bottom)
    uploadFace(faceSize, faceSize, 4);      // +Z (front)
    uploadFace(faceSize * 3, faceSize, 5);  // -Z (back)

    return cubemap;
}
Fleur::Graphics::CubemapImage Fleur::Graphics::CubemapImage::FromCross(const Image2D& src)
{
    uint32_t faceSize = src.GetWidth() / 4;

    Fleur::Graphics::CubemapImage cubemap{};
    cubemap.m_Name = src.GetName();
    cubemap.m_Width = faceSize;
    cubemap.m_Height = faceSize;
    cubemap.m_Channels = src.GetChannelsCount();
    cubemap.m_DepthBytes = src.GetDepth();
    cubemap.m_Layers = CUBEMAP_LAYERS_COUNT;

    size_t layerSize = faceSize * faceSize * cubemap.m_Channels * cubemap.m_DepthBytes;
    cubemap.m_Data.resize(layerSize * CUBEMAP_LAYERS_COUNT);

    const Fleur::Bitmap<BitmapFormat_UnsignedByte>* sourceBitmap = src.GetBitmap();
    auto uploadFace = [&](uint32_t startX, uint32_t startY, uint32_t outFace)
    {
        Fleur::Bitmap<BitmapFormat_UnsignedByte> tmp(faceSize, faceSize, cubemap.m_Channels);
        for (uint32_t y = 0; y < faceSize; ++y)
        {
            for (uint32_t x = 0; x < faceSize; ++x)
            {
                Fleur::Math::vec4 color = sourceBitmap->GetPixel(startX + x, startY + y);
                tmp.SetPixel(x, y, color);
            }
        }
        memcpy(cubemap.GetFaceData(outFace), tmp.GetData(), layerSize);
    };

    // Face indices: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
    uploadFace(faceSize * 2, faceSize, 0);  // +X (right)
    uploadFace(0, faceSize, 1);             // -X (left)
    uploadFace(faceSize, 0, 2);             // +Y (top)
    uploadFace(faceSize, faceSize * 2, 3);  // -Y (bottom)
    uploadFace(faceSize, faceSize, 4);      // +Z (front)
    uploadFace(faceSize * 3, faceSize, 5);  // -Z (back)

    return cubemap;
}

void Fleur::Graphics::CubemapImage::PostCreate(ImagePostCreation& settings)
{
    FL_CORE_ASSERT(settings.data, "[Image2D] data is nullptr");

    ImageBase::PostCreate(settings);
    // faces = reinterpret_cast<const std::array<Fleur::Graphics::Image2D, 6>*>(settings.data);
    //  uint32_t data_chank_size = settings.width * settings.height * settings.channels * settings.depth*;
    /* for (size_t i = 0; i < layers; i++)
     {
         Bitmap<BitmapFormat_UnsignedByte> bitmap = Bitmap<BitmapFormat_UnsignedByte>(settings.width, settings.height, settings.channels);
         faces[i] = Image2D(name, extension, std::move(bitmap), settings.width, settings.height, settings.channels, settings.depth);
     }

     memcpy_s(bitmap.GetData(), bitmap.GetSizeBytes(), settings.data, settings.width * settings.height * settings.channels * settings.depth);*/

    m_IsCreated = true;
}

Fleur::Graphics::SFLImageView Fleur::Graphics::CubemapImage::GetView() const
{
    return {0, (const char*)m_Data.data(), m_Width, m_Height, m_Layers, m_Channels};
}
