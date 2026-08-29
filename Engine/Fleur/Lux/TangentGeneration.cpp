#include "TangentGeneration.hpp"

#include <cstddef>
#include <cmath>
#include <optional>

namespace Fleur::Graphics
{
namespace
{
constexpr float kTangentEpsilon = 1e-8f;

struct DoubleVec3
{
    double x;
    double y;
    double z;
};

bool IsFinite(Fleur::Vec2 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool IsFinite(Fleur::Vec3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

DoubleVec3 ToDouble(Fleur::Vec3 value)
{
    return {static_cast<double>(value.x), static_cast<double>(value.y), static_cast<double>(value.z)};
}

DoubleVec3 Subtract(DoubleVec3 lhs, DoubleVec3 rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

DoubleVec3 Multiply(DoubleVec3 value, double scalar)
{
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

double Dot(DoubleVec3 lhs, DoubleVec3 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

DoubleVec3 Cross(DoubleVec3 lhs, DoubleVec3 rhs)
{
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

double Length(DoubleVec3 value)
{
    return std::hypot(value.x, std::hypot(value.y, value.z));
}

bool HasLength(DoubleVec3 value)
{
    const double length = Length(value);
    return std::isfinite(length) && length > static_cast<double>(kTangentEpsilon);
}

bool HasNonZeroLength(DoubleVec3 value)
{
    const double length = Length(value);
    return std::isfinite(length) && length > 0.0;
}

DoubleVec3 Normalize(DoubleVec3 value)
{
    const double length = Length(value);
    if (!std::isfinite(length) || length <= 0.0)
        return {0.0, 0.0, 0.0};

    return Multiply(value, 1.0 / length);
}

Fleur::Vec3 ToFloat(DoubleVec3 value)
{
    return Fleur::Vec3(
        static_cast<float>(value.x),
        static_cast<float>(value.y),
        static_cast<float>(value.z));
}

bool HasLength(Fleur::Vec3 value)
{
    return IsFinite(value) && HasLength(ToDouble(value));
}

float ComputeHandedness(DoubleVec3 normal, DoubleVec3 tangent, DoubleVec3 bitangent)
{
    if (!HasNonZeroLength(normal) || !HasNonZeroLength(tangent) || !HasNonZeroLength(bitangent))
        return 1.0f;

    const double orientation = Dot(Cross(normal, tangent), bitangent);
    return std::isfinite(orientation) && orientation < 0.0 ? -1.0f : 1.0f;
}

Fleur::Vec3 FallbackTangent(Fleur::Vec3 normal)
{
    const Fleur::Vec3 axes[] = {
        Fleur::Vec3(1.0f, 0.0f, 0.0f),
        Fleur::Vec3(0.0f, 1.0f, 0.0f),
        Fleur::Vec3(0.0f, 0.0f, 1.0f),
    };

    const DoubleVec3 normalDouble = ToDouble(normal);
    const DoubleVec3 doubleAxes[] = {
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},
    };

    std::size_t leastAlignedAxis = 0;
    double leastAlignment = std::fabs(Dot(normalDouble, doubleAxes[leastAlignedAxis]));
    for (std::size_t axis = 1; axis < 3; ++axis)
    {
        const double alignment = std::fabs(Dot(normalDouble, doubleAxes[axis]));
        if (alignment < leastAlignment)
        {
            leastAlignedAxis = axis;
            leastAlignment = alignment;
        }
    }

    const DoubleVec3 projectedAxis = Subtract(
        doubleAxes[leastAlignedAxis],
        Multiply(normalDouble, Dot(normalDouble, doubleAxes[leastAlignedAxis])));
    return ToFloat(Normalize(projectedAxis));
}
} // namespace

TangentFrame AccumulateTriangleTangent(
    Fleur::Vec3 p0, Fleur::Vec3 p1, Fleur::Vec3 p2,
    Fleur::Vec2 uv0, Fleur::Vec2 uv1, Fleur::Vec2 uv2,
    float epsilon)
{
    TangentFrame frame;

    if (!IsFinite(p0) || !IsFinite(p1) || !IsFinite(p2) ||
        !IsFinite(uv0) || !IsFinite(uv1) || !IsFinite(uv2) ||
        !std::isfinite(epsilon) || epsilon < 0.0f)
    {
        return frame;
    }

    const DoubleVec3 edge1 = Subtract(ToDouble(p1), ToDouble(p0));
    const DoubleVec3 edge2 = Subtract(ToDouble(p2), ToDouble(p0));
    const DoubleVec3 geometricNormal = Cross(edge1, edge2);
    if (!HasNonZeroLength(geometricNormal))
        return frame;

    const double duv1x = static_cast<double>(uv1.x) - static_cast<double>(uv0.x);
    const double duv1y = static_cast<double>(uv1.y) - static_cast<double>(uv0.y);
    const double duv2x = static_cast<double>(uv2.x) - static_cast<double>(uv0.x);
    const double duv2y = static_cast<double>(uv2.y) - static_cast<double>(uv0.y);
    const double determinant = duv1x * duv2y - duv1y * duv2x;

    if (!std::isfinite(determinant) || std::fabs(determinant) <= static_cast<double>(epsilon))
        return frame;

    const double reciprocal = 1.0 / determinant;
    const DoubleVec3 tangent = Multiply(
        Subtract(Multiply(edge1, duv2y), Multiply(edge2, duv1y)), reciprocal);
    const DoubleVec3 bitangent = Multiply(
        Subtract(Multiply(edge2, duv1x), Multiply(edge1, duv2x)), reciprocal);
    frame.tangent = ToFloat(tangent);
    frame.bitangent = ToFloat(bitangent);
    if (!IsFinite(frame.tangent) || !IsFinite(frame.bitangent))
        return TangentFrame{};

    frame.handedness = ComputeHandedness(geometricNormal, tangent, bitangent);
    frame.valid = true;
    return frame;
}

std::optional<Fleur::Vec4> FinalizeTangent(
    Fleur::Vec3 normal, Fleur::Vec3 accumulatedTangent,
    Fleur::Vec3 accumulatedBitangent)
{
    if (!HasLength(normal))
        return std::nullopt;

    const DoubleVec3 normalizedNormal = Normalize(ToDouble(normal));
    const DoubleVec3 tangent = Subtract(
        ToDouble(accumulatedTangent),
        Multiply(normalizedNormal, Dot(normalizedNormal, ToDouble(accumulatedTangent))));

    bool usedFallback = false;
    DoubleVec3 finalizedTangent = tangent;
    if (!HasLength(finalizedTangent))
    {
        finalizedTangent = ToDouble(FallbackTangent(ToFloat(normalizedNormal)));
        usedFallback = true;
    }
    else
        finalizedTangent = Normalize(finalizedTangent);

    const Fleur::Vec3 tangentResult = ToFloat(finalizedTangent);
    const float handedness = usedFallback ? 1.0f : ComputeHandedness(
        normalizedNormal, finalizedTangent, ToDouble(accumulatedBitangent));
    return Fleur::Vec4(tangentResult, handedness);
}

} // namespace Fleur::Graphics
