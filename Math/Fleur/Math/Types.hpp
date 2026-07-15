#pragma once

#include "Fleur/Math/Detail/GLM.hpp"

namespace Fleur::Math
{

// Transitional aliases. Keep project code dependent on Fleur::Math so the
// underlying implementation can be replaced without a project-wide rewrite.
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using IVec2 = glm::ivec2;
using IVec4 = glm::ivec4;
using UVec4 = glm::uvec4;

using Mat2 = glm::mat2;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

// TODO(Math): Remove these GLM-shaped spellings after all call sites use the
// Fleur naming convention. They are kept temporarily to make migration safe.
using vec2 = Vec2;
using vec3 = Vec3;
using vec4 = Vec4;
using ivec2 = IVec2;
using ivec4 = IVec4;
using uvec4 = UVec4;
using mat2 = Mat2;
using mat3 = Mat3;
using mat4 = Mat4;

} // namespace Fleur::Math
