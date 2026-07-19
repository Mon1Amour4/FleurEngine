#pragma once

#include "Fleur/Math/Types.hpp"

namespace Fleur::Math
{

// TODO(Math): Implement using Fleur-owned math primitives instead of GLM
// forwarding once the underlying vector and matrix storage is replaced.

// Rotate a point around the world Y axis passing through the origin.
// The angle is specified in radians.
Vec3 RotatePointY(const Vec3& point, const Vec3& center, float angleRadians);

// Build a rotation matrix around the world Y axis.
// The angle is specified in radians.
Mat4 RotateY(float angle);

}  // namespace Fleur::Math
