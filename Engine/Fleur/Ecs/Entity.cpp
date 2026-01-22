#include "Fleur/Ecs/Entity.h"

namespace Fleur::ECS
{

Entity::Entity(EntityId id) noexcept
    : m_Id(id)
{
}

EntityId Entity::GetId() const noexcept
{
    return m_Id;
}

} // namespace Fluer::ECS
