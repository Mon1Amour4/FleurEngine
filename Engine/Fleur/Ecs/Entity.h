#pragma once

#include "Core.h"
#include "Fleur/Ecs/EntityId.h"
#include "Fleur/Ecs/EntityManager.h"

namespace Fleur::ECS
{

class FLEUR_API Entity
{
    friend class EntityManager::EntityManagerImpl;

public:
    EntityId GetId() const noexcept;

    ~Entity() = default;

protected:
    explicit Entity(EntityId id) noexcept;

private:
    EntityId m_Id;    
};

}  // namespace Fluer::ECS
