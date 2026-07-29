#pragma once

// TODO(Math): Replace this implementation include with Fleur-owned math
// primitives once the public API no longer needs GLM-compatible storage.
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
