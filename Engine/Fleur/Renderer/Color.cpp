#include "Color.h"

#include <algorithm>

// Little endian
// [3][2][1][0]
// [A][B][G][R]


Fleur::Graphics::Color::Color(float r, float g, float b)
    : a(1.f)
    , b(b)
    , g(g)
    , r(r)
{
}

Fleur::Graphics::Color::Color(float r, float g, float b, float a)
    : a(a)
    , b(b)
    , g(g)
    , r(r)
{
}

uint32_t Fleur::Graphics::Color::ToRGBA8() const
{
    uint8_t a8 = std::clamp<float>(a, 0, 1.f) * 255 + 0.5f;
    uint8_t b8 = std::clamp<float>(b, 0, 1.f) * 255 + 0.5f;
    uint8_t g8 = std::clamp<float>(g, 0, 1.f) * 255 + 0.5f;
    uint8_t r8 = std::clamp<float>(r, 0, 1.f) * 255 + 0.5f;

    return (((uint32_t)r8 << 0) | ((uint32_t)g8 << 8) | ((uint32_t)b8 << 16) | ((uint32_t)a8 << 24));
}
