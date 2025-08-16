#pragma once
#include <glm/ext/vector_float4.hpp>

namespace Fleur
{

template <typename Derived>
struct BitmapFormat_UnsignedByte
{
    void SetPixel(uint32_t x, uint32_t y, const glm::vec4& pixel_data)
    {
        Derived& underlying = static_cast<Derived&>(*this);

        glm::vec4 data = underlying.GetPixel(x, y);

        uint8_t* byte_data = reinterpret_cast<uint8_t*>(underlying.Data());

        uint32_t comp = underlying.Components();
        const int offset = comp * (y * underlying.Width() + x);
        if (comp > 0)
            byte_data[offset + 0] = uint8_t(pixel_data.x * 255.0f);
        if (comp > 1)
            byte_data[offset + 1] = uint8_t(pixel_data.y * 255.0f);
        if (comp > 2)
            byte_data[offset + 2] = uint8_t(pixel_data.z * 255.0f);
        if (comp > 3)
            byte_data[offset + 3] = uint8_t(pixel_data.w * 255.0f);
    }

    glm::vec4 GetPixel(uint32_t x, uint32_t y) const
    {
        const Derived& underlying = static_cast<const Derived&>(*this);
        const uint8_t* byte_data = reinterpret_cast<const uint8_t*>(underlying.Data());
        uint32_t comp = underlying.Components();

        const int offset = comp * (y * underlying.Width() + x);
        return glm::vec4(comp > 0 ? float(byte_data[offset + 0]) / 255.0f : 0.0f, comp > 1 ? float(byte_data[offset + 1]) / 255.0f : 0.0f,
                         comp > 2 ? float(byte_data[offset + 2]) / 255.0f : 0.0f, comp > 3 ? float(byte_data[offset + 3]) / 255.0f : 0.0f);
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

        float* float_data = reinterpret_cast<float*>(underlying.Data());
        if (comp > 0)
            float_data[offset + 0] = pixel_data.x;
        if (comp > 1)
            float_data[offset + 1] = pixel_data.y;
        if (comp > 2)
            float_data[offset + 2] = pixel_data.z;
        if (comp > 3)
            float_data[offset + 3] = pixel_data.w;
    }

    glm::vec4 GetPixel(uint32_t x, uint32_t y) const
    {
        Derived& underlying = static_cast<Derived&>(*this);

        uint32_t comp = underlying.Components();

        const int offset = comp * (y * underlying.Width() + x);
        const float* float_data = reinterpret_cast<const float*>(underlying.Data());
        return glm::vec4(comp > 0 ? float_data[offset + 0] : 0.0f, comp > 1 ? float_data[offset + 1] : 0.0f, comp > 2 ? float_data[offset + 2] : 0.0f,
                         comp > 3 ? float_data[offset + 3] : 0.0f);
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
        : w(0)
        , h(0)
        , comp(0)
    {
    }

    ~Bitmap() = default;

    Bitmap<Fmt>(const Bitmap<Fmt>& other) = delete;
    Bitmap<Fmt>& operator=(const Bitmap<Fmt>& other) = delete;

    Bitmap<Fmt>(Bitmap<Fmt>&& other) noexcept
    {
        w = other.w;
        h = other.h;
        comp = other.comp;
        data = std::move(other.data);

        other.w = 0;
        other.h = 0;
        other.comp = 0;
    }
    Bitmap<Fmt>& operator=(Bitmap<Fmt>&& other) noexcept
    {
        if (this != &other)
        {
            w = other.w;
            h = other.h;
            comp = other.comp;
            data = std::move(other.data);

            other.w = 0;
            other.h = 0;
            other.comp = 0;
        }

        return *this;
    }

    Bitmap(const void* in_data, uint32_t width, uint32_t height, uint32_t channels)
        : w(width)
        , h(height)
        , comp(channels)
        , data(width * height * channels * Fmt<Bitmap>::GetBytesPerComponent())
    {
        memcpy_s(data.data(), data.size(), in_data, data.size());
    }
    Bitmap(uint32_t width, uint32_t height, uint32_t components)
        : w(width)
        , h(height)
        , comp(components)
        , data(width * height * components * Fmt<Bitmap>::GetBytesPerComponent())
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
        return w;
    }

    uint32_t Height() const
    {
        return h;
    }

    uint32_t Components() const
    {
        return comp;
    }

    const void* Data() const
    {
        return reinterpret_cast<const void*>(data.data());
    }
    void* Data()
    {
        return reinterpret_cast<void*>(data.data());
    }


    uint32_t GetSizeBytes() const
    {
        return data.size();
    }

private:
    std::vector<uint8_t> data;
    uint32_t w;
    uint32_t h;
    uint32_t comp;
};
}  // namespace Fleur
