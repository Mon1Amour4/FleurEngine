#include <atomic>

#include "Fleur/Ecs/Entity.h"
#include "Fleur/Ecs/EntityManager.h"

namespace Fleur::ECS
{

class EntityManager::EntityManagerImpl
{
public:
    EntityManagerImpl() = default;

    void OnInitImpl()
    {
        m_LastEntityId = 1;
    }

    void OnShutdownImpl()
    {
    }

    Entity CreateEntityImpl() noexcept
    {
        EntityId id = m_LastEntityId.fetch_add( 1, std::memory_order_relaxed);

        assert(id <= EntityManager::MAX_ENTITIES);

        return Entity( id );
    }

    void RemoveEntityImpl(Entity entity)
    {
        UNUSED(entity);
    }

    EntityId GetLastEntityIdImpl() const noexcept
    {
        return m_LastEntityId;
    }

    static std::atomic<EntityId> m_LastEntityId;
};

EntityManager::EntityManager()
    : d(new EntityManagerImpl())
{
		
}

EntityManager::~EntityManager()
{
    delete d;
}

void EntityManager::OnInit()
{
    d->OnInitImpl();
}

void EntityManager::OnShutdown()
{
    d->OnShutdownImpl();
}

Entity EntityManager::CreateEntity() noexcept
{
    return d->CreateEntityImpl();
}

void EntityManager::RemoveEntity(Entity& entity)
{
    return d->RemoveEntityImpl(entity);
}

EntityId EntityManager::GetLastEntityId() const noexcept
{
    return d->GetLastEntityIdImpl();
}

} // namespace Fleur::ECS
