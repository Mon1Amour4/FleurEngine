#pragma once

#include <cmath>
#include <array>
#include <algorithm>

namespace Fleur::ShadowMath
{
struct CascadeOverlap
{
    float start{};
    float end{};
};

template <std::size_t Count>
inline CascadeOverlap GetCascadeOverlap(const std::array<float, Count>& splits, std::size_t cascadeIndex, float startFraction, float)
{
    if (Count == 0 || cascadeIndex == 0 || cascadeIndex >= Count)
        return {};

    // The last cascade has no following split to overlap with.
    if (cascadeIndex + 1 >= Count)
        return {splits[cascadeIndex], splits[cascadeIndex]};

    const float previousSplit = splits[cascadeIndex - 1];
    const float currentSplit = splits[cascadeIndex];
    const float fraction = std::clamp(startFraction, 0.0f, 1.0f);
    const float start = previousSplit + (currentSplit - previousSplit) * fraction;
    return {start, currentSplit};
}

inline float SnapToTexel(float coordinate, float texelSize)
{
    if (!(texelSize > 0.0f) || !std::isfinite(coordinate))
        return coordinate;
    return std::round(coordinate / texelSize) * texelSize;
}

} // namespace Fleur::ShadowMath
