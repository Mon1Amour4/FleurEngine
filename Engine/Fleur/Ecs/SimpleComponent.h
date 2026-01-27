#pragma once

#include <cstdint>
#include <limits>
#include <array>
#include <type_traits>

#include "Fleur/Core.h"
#include "Fleur/Ecs/EntityId.h"
#include "Fleur/Ecs/Entity.h"

namespace Fleur::ECS
{

template<typename T>
class SimpleComponent
{
public:
    using value_type = std::remove_cvref_t<T>;
    using index_type = uint32_t;
    using size_type  = index_type;
    using ref_type   = value_type&;
    using cref_type  = value_type const &;
    using ptr_type   = value_type*;
    using cptr_type  = value_type const *;

    static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

    SimpleComponent() noexcept;
    ~SimpleComponent() = default;

    FLEUR_NON_COPYABLE(SimpleComponent)

    size_type Size() const noexcept;

    bool HasEntity(EntityId id) const noexcept;

    template<typename... Args>
    void AddEntity(EntityId id, Args&&... args);
    void RemoveEntity(EntityId id);

    ref_type  Get(EntityId id);
    cref_type Get(EntityId id) const;

private:
    struct storage
    {
        alignas(value_type) uint8_t data[sizeof(value_type)];
    };

    std::array<index_type, EntityManager::MAX_ENTITIES> m_Sparse;
    std::array<storage, EntityManager::MAX_ENTITIES> m_Dense; 
    std::array<EntityId, EntityManager::MAX_ENTITIES> m_DenseEntities;

    size_type m_Size;

    inline ptr_type ptr(index_type idx) noexcept;
    inline cptr_type ptr(index_type idx) const noexcept;
};

}  // namespace Fleur::ECS

#include "Fleur/Ecs/SimpleComponent.hpp"
