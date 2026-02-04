#pragma once

#include <cstdint>
#include <limits>

namespace Fleur::ECS
{

using EntityId = uint32_t;
inline constexpr EntityId INVALID_ENTITY_ID = std::numeric_limits<EntityId>::max();
inline constexpr EntityId MAX_ENTITY_ID = 1024 * 512; // 2^19

} // namespace Fleur::ECS
