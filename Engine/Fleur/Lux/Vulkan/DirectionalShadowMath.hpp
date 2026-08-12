#pragma once

#include <cmath>

namespace Fleur::ShadowMath
{
inline float SnapToTexel(float coordinate, float texelSize)
{
    if (!(texelSize > 0.0f) || !std::isfinite(coordinate))
        return coordinate;
    return std::round(coordinate / texelSize) * texelSize;
}

} // namespace Fleur::ShadowMath
