#include "Color.h"

Fuego::Graphics::Color::Color(uint8_t val, uint8_t val_2, uint8_t val_3, uint8_t val_4)
{
    data = (val << 0) | (val_2 << 8) | (val_3 << 16) | (val_4 << 24);
}

Fuego::Graphics::Color::Color(float val, float val_2, float val_3, float val_4)
{
    data = (convert_float_to_byte(val) << 0) | (convert_float_to_byte(val_2) << 8) |
           (convert_float_to_byte(val_3) << 16) | (convert_float_to_byte(val_4) << 24);
    int a = 5;
}

uint32_t Fuego::Graphics::Color::Channels(const Color& color)
{
    uint32_t count = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        if ((color.data >> (i * 8)) & 0xFF)
            ++count;
    }
    return count;
}

uint8_t Fuego::Graphics::Color::convert_float_to_byte(float val) const
{
    return static_cast<uint8_t>(val * 255.0f + 0.5f);
}
