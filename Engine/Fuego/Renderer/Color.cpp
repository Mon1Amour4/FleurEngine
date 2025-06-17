#include "Color.h"

Fuego::Graphics::Color::Color(uint8_t val, uint8_t val_2, uint8_t val_3, uint8_t val_4)
{
    data = (val << 0) | (val_2 << 8) | (val_3 << 16) | (val_4 << 24);
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
