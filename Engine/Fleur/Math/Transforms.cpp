#include "Fleur/Math/Transforms.hpp"

namespace Fleur::Math
{

Vec3 RotatePointY(const Vec3& point, const Vec3& center, float angleRadians)
{
    float x = point.x - center.x;
    float z = point.z - center.z;

    float newX = x * std::cos(angleRadians) - z * std::sin(angleRadians);
    float newZ = x * std::sin(angleRadians) + z * std::cos(angleRadians);

    return {newX + center.x, point.y, newZ + center.z};
}

// TODO(Math): Implement RotateY.

}  // namespace Fleur::Math
