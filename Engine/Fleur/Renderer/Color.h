#pragma once

namespace Fleur::Graphics
{

class Color
{
public:
    Color(int r, int g = 0, int b = 0, int a = 0);
    Color(float r, float g = 0.f, float b = 0.f, float a = 0.f);
    ~Color() = default;

    static uint32_t Channels(const Color& color);
    uint32_t Data() const
    {
        return static_cast<uint32_t>(m_Data[0]) | (static_cast<uint32_t>(m_Data[1]) << 8) | (static_cast<uint32_t>(m_Data[2]) << 16) |
               (static_cast<uint32_t>(m_Data[3]) << 24);
    }

private:
    std::array<uint8_t, 4> m_Data;

    uint8_t ConvertFloatToByte(float val) const;
};

}  // namespace Fleur::Graphics
