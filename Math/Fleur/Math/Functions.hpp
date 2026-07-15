#pragma once

#include "Fleur/Math/Types.hpp"

namespace Fleur::Math
{

// Transitional forwarding surface for existing engine math calls.
// TODO(Math): Replace these using-declarations with Fleur-owned functions and
// establish consistent naming (for example RotatePointY, Normalize, Clamp).
using glm::clamp;
using glm::cross;
using glm::degrees;
using glm::dot;
using glm::eulerAngles;
using glm::fclamp;
using glm::identity;
using glm::inverse;
using glm::length;
using glm::lookAt;
using glm::make_mat4;
using glm::max;
using glm::min;
using glm::normalize;
using glm::orthoRH_ZO;
using glm::perspective;
using glm::pi;
using glm::quat_cast;
using glm::radians;
using glm::rotate;
using glm::sqrt;
using glm::transpose;

} // namespace Fleur::Math
