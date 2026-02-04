#pragma once

#include <cstdint>
#include <limits>
#include <array>
#include <type_traits>
#include <iterator>

#include "Fleur/Core.h"
#include "Fleur/Ecs/EntityId.h"

namespace Fleur::ECS
{

/// Component is NOT thread safe.
///
/// Ownership model:
///     One thread owns one component type and is the only writer.
///
/// Allowed:
///     Different threads operate on different component types.
///
/// Forbidden:
///     - Multiple threads accessing the same Component<T>
///       when at least one thread performs modifications.
///     - Reading components while another thread calls
///       Emplace, Set or Remove.
///     - Keeping pointers/references across modification phases.
/// 
/// If you want to use one thread as writer and multiple as readers - separate writing and reading phases
/// 
/// Incorrect usage:
///     thread 1 -> Component A, Component B, Component C
///     thread 2 -> Component A, Component B, Component C
///     thread 3 -> Component A, Component B, Component C
/// 
/// Correct usage:
///     thread 1 -> Component A
///     thread 2 -> Component B
///     thread 3 -> Component C


/// <summary>
/// Caution! Component is not thread safe structure
/// </summary>
/// <typeparam name="T">entity component type, e.g. vec3f, single mesh or texture</typeparam>
template<typename T>
class Component
{
    static_assert(std::is_nothrow_copy_constructible_v<T>);
    static_assert(std::is_nothrow_copy_assignable_v<T>);
public:
    using value_type = std::remove_cvref_t<T>;
    using pointer_type = value_type *;
    using const_pointer_type = value_type const *;

    using size_type = uint32_t;
    using index_type = uint32_t;
    
    using iterator = pointer_type;
    using const_iterator = const_pointer_type;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using difference_type = std::ptrdiff_t;

    static constexpr size_type MAX_COUNT = 1024 * 512; // 2^19
    static constexpr index_type INVALID_INDEX = std::numeric_limits<index_type>::max();

    Component() noexcept;
    ~Component() noexcept;

    FLEUR_NON_COPYABLE_NON_MOVABLE(Component);

    inline size_type Size() const noexcept { return m_Size; }

    template<typename... Args>
    pointer_type Emplace(EntityId id, Args&&... args) noexcept requires std::is_nothrow_constructible_v<typename value_type, Args...>;

    pointer_type Set(EntityId id, T&&) noexcept;

    pointer_type Get(EntityId id) noexcept;
    const_pointer_type Get(EntityId id) const noexcept;

    bool Has(EntityId id) const noexcept;

    bool Remove(EntityId id) noexcept;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;
    reverse_iterator rbegin() noexcept;
    reverse_iterator rend() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator rend() const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    const_reverse_iterator crend() const noexcept;

private:
    union _ProxyContainer
    {
        value_type data;
        _ProxyContainer() {}
        ~_ProxyContainer() {}
    };

    using DataContainer = std::array<_ProxyContainer, MAX_COUNT>;
    using IndexEntityContainer = std::array<index_type, MAX_COUNT>;
    using EntityIndexContainer = std::array<EntityId, MAX_ENTITY_ID>;

    DataContainer m_Data = {};
    IndexEntityContainer m_EntityOf = {};
    EntityIndexContainer m_IndexOf = {};
    size_type m_Size = 0;
};

}  // namespace Fleur::ECS

#include "Fleur/Ecs/Component.hpp"
