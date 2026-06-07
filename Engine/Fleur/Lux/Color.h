#pragma once

namespace Fleur::Graphics
{

// [0;1] - defalt values are normilized to 1, but for HDR we can have more than 1
// Little endian
// [3][2][1][0]
// [A][B][G][R]
class Color
{
public:
    Color() = default;
    ~Color() = default;

    Color(float r, float g, float b);
    Color(float r, float g, float b, float a);

    static uint32_t GetChannels()
    {
        return 4;
    }

    uint32_t ToRGBA8() const;

#pragma region Predefined colors
    static Color Red()
    {
        return Color(1.f, 0.f, 0.f, 1.f);
    }
    static Color Green()
    {
        return Color(0.f, 1.f, 0.f, 1.f);
    }
    static Color Blue()
    {
        return Color(0.f, 0.f, 1.f, 1.f);
    }
    static Color White()
    {
        return Color(1.f, 1.f, 1.f, 1.f);
    }
    static Color Black()
    {
        return Color(0.f, 0.f, 0.f, 1.f);
    }
    static Color Yellow()
    {
        return Color(1.f, 1.f, 0.f, 1.f);
    }
    static Color Cyan()
    {
        return Color(0.f, 1.f, 1.f, 1.f);
    }
    static Color Magenta()
    {
        return Color(1.f, 0.f, 1.f, 1.f);
    }
    static Color Transparent()
    {
        return Color(0.f, 0.f, 0.f, 0.f);
    }
    static Color Gray(float intensity)
    {
        return Color(intensity, intensity, intensity, 1.f);
    }
#pragma endregion

private:
    float r{1}, g{1}, b{1}, a{1};
};

}  // namespace Fleur::Graphics
