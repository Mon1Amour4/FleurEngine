#pragma once

#include <algorithm>
#include <memory>

#include "Fleur/Concepts.hpp"

#include "Fleur/Ecs/AbstractComponent.h"

namespace Fleur::ECS
{

template <typename T>
AbstractComponent<T>::AbstractComponent() noexcept
    : m_Sparse()
    , m_Dense()
    , m_DenseEntities()
    , m_Size( 0 )
{
    std::fill(m_Sparse.begin(), m_Sparse.end(), AbstractComponent<T>::INVALID_INDEX);
}

template <typename T>
inline AbstractComponent<T>::size_type AbstractComponent<T>::Size() const noexcept
{
    return m_Size;
}

template <typename T>
inline bool AbstractComponent<T>::HasEntity(EntityId id) const noexcept
{
    return m_Sparse[ id ] != AbstractComponent<T>::INVALID_INDEX;
}

template <typename T>
inline AbstractComponent<T>::ptr_type AbstractComponent<T>::ptr(index_type idx) noexcept
{
    return std::launder( reinterpret_cast<ptr_type>(m_Dense[idx].data) );
}

template <typename T>
inline AbstractComponent<T>::cptr_type AbstractComponent<T>::ptr(index_type idx) const noexcept
{
    return std::launder(reinterpret_cast<cptr_type>(m_Dense[idx].data));
}

template<typename T>
template<typename... Args>
inline void AbstractComponent<T>::AddEntity(EntityId id, Args&&... args)
{
    assert(m_Size < EntityManager::MAX_ENTITIES); 
    assert(!HasEntity(id));

    index_type idx = m_Size;

    m_Sparse[id] = idx;
    m_DenseEntities[idx] = id;

    std::construct_at<value_type>(ptr(idx), std::forward<Args>(args)...);

    ++m_Size;
}

template <typename T>
inline void AbstractComponent<T>::RemoveEntity(EntityId id)
{
    assert(m_Sparse[id] != INVALID_INDEX);

    index_type idx  = m_Sparse[id];
    index_type last = m_Size - 1;

    if (idx != last)
    {
        std::swap(*ptr(idx), *ptr(last));

        EntityId moved = m_DenseEntities[last];

        m_DenseEntities[idx] = moved;
        m_Sparse[moved] = idx;
    }

    m_Sparse[id] = AbstractComponent<T>::INVALID_INDEX;

    --m_Size;

    std::destroy_at<value_type>(ptr(last));
}

template <typename T>
inline AbstractComponent<T>::ref_type AbstractComponent<T>::Get(EntityId id)
{
    assert(m_Sparse[id] != INVALID_INDEX);
    return *ptr(m_Sparse[id]);
}

template <typename T>
inline AbstractComponent<T>::cref_type AbstractComponent<T>::Get(EntityId id) const
{
    assert(m_Sparse[id] != INVALID_INDEX);
    return *ptr(m_Sparse[id]);
}

} // namespace Fleur::ECS
