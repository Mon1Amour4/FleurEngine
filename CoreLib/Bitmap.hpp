#pragma once
#include <glm/ext/vector_float4.hpp>

#include "Allocator.h"

namespace Fleur
{

template <typename Derived>
struct BitmapFormat_UnsignedByte
{
    void SetPixel(uint32_t x, uint32_t y, const glm::vec4& pixelData)
    {
        Derived& underlying = static_cast<Derived&>(*this);

        glm::vec4 data = underlying.GetPixel(x, y);

        uint8_t* byteData = reinterpret_cast<uint8_t*>(underlying.Data());

        uint32_t comp = underlying.Components();
        const int offset = comp * (y * underlying.Width() + x);
        if (comp > 0)
            byteData[offset + 0] = uint8_t(pixelData.x * 255.0f);
        if (comp > 1)
            byteData[offset + 1] = uint8_t(pixelData.y * 255.0f);
        if (comp > 2)
            byteData[offset + 2] = uint8_t(pixelData.z * 255.0f);
        if (comp > 3)
            byteData[offset + 3] = uint8_t(pixelData.w * 255.0f);
    }

    glm::vec4 GetPixel(uint32_t x, uint32_t y) const
    {
        const Derived& underlying = static_cast<const Derived&>(*this);
        const uint8_t* byteData = reinterpret_cast<const uint8_t*>(underlying.Data());
        uint32_t comp = underlying.Components();

        const int offset = comp * (y * underlying.Width() + x);
        return glm::vec4(comp > 0 ? float(byteData[offset + 0]) / 255.0f : 0.0f, comp > 1 ? float(byteData[offset + 1]) / 255.0f : 0.0f,
                         comp > 2 ? float(byteData[offset + 2]) / 255.0f : 0.0f, comp > 3 ? float(byteData[offset + 3]) / 255.0f : 0.0f);
    }

protected:
    uint32_t GetBytesPerComponent() const
    {
        return 1;
    }
};

template <typename Derived>
struct BitmapFormat_Float
{
    void SetPixel(uint32_t x, uint32_t y, const glm::vec4& pixel_data)
    {
        Derived& underlying = static_cast<Derived&>(*this);

        uint32_t comp = underlying.Components();
        const int offset = comp * (y * underlying.Width() + x);

        float* floatData = reinterpret_cast<float*>(underlying.Data());
        if (comp > 0)
            floatData[offset + 0] = pixel_data.x;
        if (comp > 1)
            floatData[offset + 1] = pixel_data.y;
        if (comp > 2)
            floatData[offset + 2] = pixel_data.z;
        if (comp > 3)
            floatData[offset + 3] = pixel_data.w;
    }

    glm::vec4 GetPixel(uint32_t x, uint32_t y) const
    {
        Derived& underlying = static_cast<Derived&>(*this);

        uint32_t comp = underlying.Components();

        const int offset = comp * (y * underlying.Width() + x);
        const float* floatData = reinterpret_cast<const float*>(underlying.Data());
        return glm::vec4(comp > 0 ? floatData[offset + 0] : 0.0f, comp > 1 ? floatData[offset + 1] : 0.0f, comp > 2 ? floatData[offset + 2] : 0.0f,
                         comp > 3 ? floatData[offset + 3] : 0.0f);
    }

protected:
    uint32_t GetBytesPerComponent() const
    {
        return sizeof(float);
    }
};


template <template <typename> class Fmt>
class Bitmap : public Fmt<Bitmap<Fmt>>
{
public:
    Bitmap()
        : m_Width(0)
        , m_Height(0)
        , m_Components(0)
    {
    }

    ~Bitmap() = default;

    Bitmap<Fmt>(const Bitmap<Fmt>& other) = delete;
    Bitmap<Fmt>& operator=(const Bitmap<Fmt>& other) = delete;

    Bitmap<Fmt>(Bitmap<Fmt>&& other) noexcept
    {
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_Components = other.m_Components;
        m_Data = std::move(other.m_Data);

        other.m_Width = 0;
        other.m_Height = 0;
        other.m_Components = 0;
    }
    Bitmap<Fmt>& operator=(Bitmap<Fmt>&& other) noexcept
    {
        if (this != &other)
        {
            m_Width = other.m_Width;
            m_Height = other.m_Height;
            m_Components = other.m_Components;
            m_Data = std::move(other.m_Data);

            other.m_Width = 0;
            other.m_Height = 0;
            other.m_Components = 0;
        }

        return *this;
    }

    Bitmap(const void* IN inData, uint32_t width, uint32_t height, uint32_t channels)
        : m_Width(width)
        , m_Height(height)
        , m_Components(channels)
        , m_Data(width * height * channels * Fmt<Bitmap>::GetBytesPerComponent())
    {
        memcpy_s(m_Data.data(), m_Data.size(), inData, m_Data.size());
    }
    Bitmap(uint32_t width, uint32_t height, uint32_t components)
        : m_Width(width)
        , m_Height(height)
        , m_Components(components)
        , m_Data(width * height * components * Fmt<Bitmap>::GetBytesPerComponent())
    {
    }

    void SetPixel(uint32_t x, uint32_t y, const glm::vec4& pixel_data)
    {
        Fmt<Bitmap>::SetPixel(x, y, pixel_data);
    }

    glm::vec4 GetPixel(uint32_t x, uint32_t y) const
    {
        return Fmt<Bitmap>::GetPixel(x, y);
    }

    uint32_t Width() const
    {
        return m_Width;
    }

    uint32_t Height() const
    {
        return m_Height;
    }

    uint32_t Components() const
    {
        return m_Components;
    }

    const void* Data() const
    {
        return reinterpret_cast<const void*>(m_Data.data());
    }
    void* Data()
    {
        return reinterpret_cast<void*>(m_Data.data());
    }


    size_t GetSizeBytes() const
    {
        return m_Data.size();
    }

private:
    std::vector<uint8_t, Fleur::Core::CustomAllocator<uint8_t>> m_Data;
    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_Components;
};
}  // namespace Fleur
