#pragma once

#include "Fleur/Math/Types.hpp"

#include <optional>

namespace Fleur::Graphics
{

struct TangentFrame
{
    Fleur::Vec3 tangent{0.0f};
    Fleur::Vec3 bitangent{0.0f};
    // Orientation of the tangent/bitangent pair around the geometric normal.
    float handedness{1.0f};
    // True when tangent and bitangent contain a finite raw contribution.
    bool valid{false};
};

// Returns raw, unnormalized tangent and bitangent contributions for one triangle.
// UV determinants with absolute value <= epsilon are rejected as degenerate.
TangentFrame AccumulateTriangleTangent(
    Fleur::Vec3 p0, Fleur::Vec3 p1, Fleur::Vec3 p2,
    Fleur::Vec2 uv0, Fleur::Vec2 uv1, Fleur::Vec2 uv2,
    float epsilon = 1e-8f);

// Orthonormalizes accumulated tangent data against normal and returns handedness.
// If the accumulation is unusable, substitutes a deterministic tangent with
// handedness +1.
std::optional<Fleur::Vec4> FinalizeTangent(
    Fleur::Vec3 normal, Fleur::Vec3 accumulatedTangent,
    Fleur::Vec3 accumulatedBitangent);

} // namespace Fleur::Graphics
