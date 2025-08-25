#include "Color.h"

Fleur::Graphics::Color::Color(int r, int g, int b, int a)
{
    m_Data[0] = static_cast<uint8_t>(r);
    m_Data[1] = static_cast<uint8_t>(g);
    m_Data[2] = static_cast<uint8_t>(b);
    m_Data[3] = static_cast<uint8_t>(a);
}

Fleur::Graphics::Color::Color(float r, float g, float b, float a)
{
    m_Data[0] = ConvertFloatToByte(r);
    m_Data[1] = ConvertFloatToByte(g);
    m_Data[2] = ConvertFloatToByte(b);
    m_Data[3] = ConvertFloatToByte(a);
}

uint32_t Fleur::Graphics::Color::Channels(const Color& color)
{
    uint32_t count = 4;
    for (size_t i = 3; i >= 0; i--)
    {
        if (color.m_Data[i] > 0)
            return count;
        --count;
    }
}

uint8_t Fleur::Graphics::Color::ConvertFloatToByte(float val) const
{
    return static_cast<uint8_t>(val * 255.0f + 0.5f);
}
