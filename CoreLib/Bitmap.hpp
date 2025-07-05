#pragma once
#include <glm/ext/vector_float4.hpp>

namespace Fuego
{

template <typename Derived>
struct BitmapFormat_UnsignedByte
{
    void SetPixel(uint32_t x, uint32_t y, const glm::vec4& pixel_data)
    {
        Derived& underlying = static_cast<Derived&>(*this);

        glm::vec4 data = underlying.GetPixel(x, y);

        uint8_t* byte_data = underlying.Data();

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

    glm::vec4 GetPixel(uint32_t x, uint32_t y)
    {
        Derived& underlying = static_cast<Derived&>(*this);
        uint8_t* byte_data = underlying.Data();
        uint32_t comp = underlying.Components();

        const int offset = comp * (y * underlying.Width() + x);
        return glm::vec4(comp > 0 ? float(byte_data[offset + 0]) / 255.0f : 0.0f, comp > 1 ? float(byte_data[offset + 1]) / 255.0f : 0.0f,
                         comp > 2 ? float(byte_data[offset + 2]) / 255.0f : 0.0f, comp > 3 ? float(byte_data[offset + 3]) / 255.0f : 0.0f);
    }

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

    glm::vec4 GetPixel(uint32_t x, uint32_t y)
    {
        Derived& underlying = static_cast<Derived&>(*this);

        uint32_t comp = underlying.Components();

        const int offset = comp * (y * underlying.Width() + x);
        const float* float_data = reinterpret_cast<const float*>(underlying.Data());
        return glm::vec4(comp > 0 ? float_data[offset + 0] : 0.0f, comp > 1 ? float_data[offset + 1] : 0.0f, comp > 2 ? float_data[offset + 2] : 0.0f,
                         comp > 3 ? float_data[offset + 3] : 0.0f);
    }

    uint32_t GetBytesPerComponent() const
    {
        return sizeof(float);
    }
};


template <template <typename> class Fmt>
class Bitmap : public Fmt<Bitmap<Fmt>>
{
public:
    Bitmap() = default;
    Bitmap(const Fuego::Graphics::Image2D* img)
        : w(img->Width())
        , h(img->Height())
        , comp(img->Channels())
        , data(img->Width() * img->Height() * img->Channels() * Fmt<Bitmap>::GetBytesPerComponent())
    {
        memcpy_s(data.data(), data.size(), img->Data(), img->SizeBytes());
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

    glm::vec4 GetPixel(uint32_t x, uint32_t y)
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

    uint8_t* Data()
    {
        return data.data();
    }

private:
    std::vector<uint8_t> data;
    uint32_t w;
    uint32_t h;
    uint32_t comp;
};
}  // namespace Fuego
