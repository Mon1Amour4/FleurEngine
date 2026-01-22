#pragma once

#include "Services/ServiceInterfaces.hpp"
#include "Fleur/Ecs/EntityId.h"

namespace Fleur::ECS
{

class Entity;

class FLEUR_API EntityManager : public Service<EntityManager>
{
public:
    EntityManager();
    ~EntityManager();

    void OnInit();
    void OnShutdown();

    static constexpr EntityId MAX_ENTITIES = 1048576;

    Entity CreateEntity() noexcept;
    void RemoveEntity(Entity& entity);

    EntityId GetLastEntityId() const noexcept;

    class EntityManagerImpl;

private:
    EntityManagerImpl* d;
};

} // namespace Fleur::ECS
