#pragma once

#include <algorithm>
#include <memory>
#include <ranges>

#include "Fleur/Ecs/Component.h"

namespace Fleur::ECS
{

template <typename T>
inline Component<T>::Component() noexcept
{
	std::ranges::fill(m_IndexOf, INVALID_INDEX);
	std::ranges::fill(m_EntityOf, INVALID_ENTITY_ID);
}

template <typename T>
inline Component<T>::~Component() noexcept
{
    for (index_type idx = 0; idx < m_Size; ++idx)
    {
        std::destroy_at(std::addressof(m_Data[idx].data));
    }
}

template <typename T>
template <typename... Args>
inline Component<T>::pointer_type Component<T>::Emplace(EntityId id, Args&&... args) noexcept
requires std::is_nothrow_constructible_v<typename value_type, Args...>
{
	if (id == INVALID_ENTITY_ID || id >= MAX_ENTITY_ID)
	{
		return nullptr;
	}

	if (m_IndexOf[id] != INVALID_INDEX)
	{
		return Get(id);
	}

	index_type idx = m_Size;
	if (idx >= MAX_COUNT)
	{
		return nullptr;
	}

	pointer_type ptr = std::construct_at(std::addressof(m_Data[idx].data), std::forward<Args>(args));
	m_IndexOf[id] = idx;
	m_EntityOf[idx] = id;

	++m_Size;

	return ptr;
}

template <typename T>
inline Component<T>::pointer_type Component<T>::Set(EntityId id, T&& value) noexcept
{
	if (id == INVALID_ENTITY_ID || id >= MAX_ENTITY_ID)
	{
		return nullptr;
	}

	if (m_IndexOf[id] != INVALID_INDEX)
	{
		index_type idx = m_IndexOf[id];
		m_Data[idx].data = std::move_if_noexcept(value);

		return std::addressof(m_Data[idx].data);
	}
	else
	{
		index_type idx = m_Size;
		if (idx >= MAX_COUNT)
		{
			return nullptr;
		}

		pointer_type ptr = std::construct_at(std::addressof(m_Data[idx].data), std::move_if_noexcept(value));
		m_IndexOf[id] = idx;
		m_EntityOf[idx] = id;

		++m_Size;

		return ptr;
	}
}

template <typename T>
inline Component<T>::pointer_type Component<T>::Get(EntityId id) noexcept
{
	if (id == INVALID_ENTITY_ID || id >= MAX_ENTITY_ID)
	{
		return nullptr;
	}

	index_type idx = m_IndexOf[id];

	if (idx == INVALID_INDEX)
	{
		return nullptr;
	}

	return std::addressof(m_Data[idx].data);
}

template <typename T>
inline Component<T>::const_pointer_type Component<T>::Get(EntityId id) const noexcept
{
	if (id == INVALID_ENTITY_ID || id >= MAX_ENTITY_ID)
	{
		return nullptr;
	}

	index_type idx = m_IndexOf[id];

	if (idx == INVALID_INDEX)
	{
		return nullptr;
	}

	return std::addressof(m_Data[idx].data);
}

template <typename T>
inline bool Component<T>::Has(EntityId id) const noexcept
{
	if (id == INVALID_ENTITY_ID || id >= MAX_ENTITY_ID)
	{
		return false;
	}

	index_type idx = m_IndexOf[id];

	return idx != INVALID_INDEX && m_EntityOf[idx] == id;
}

template <typename T>
inline bool Component<T>::Remove(EntityId id) noexcept
{
    if (m_Size == 0)
    {
        return false;
    }

	if (id == INVALID_ENTITY_ID || id >= MAX_ENTITY_ID)
	{
		return false;
	}

	index_type idx = m_IndexOf[id];

	if (idx == INVALID_INDEX)
	{
		return false;
	}

	index_type last = m_Size - 1;

	if (idx != last)
	{
		EntityId move = m_EntityOf[last];

		m_Data[idx].data = std::move_if_noexcept(m_Data[last].data);
		m_IndexOf[move] = idx;
		m_EntityOf[idx] = move;
	}

	std::destroy_at(std::addressof(m_Data[last].data));

	m_IndexOf[id] = INVALID_INDEX;
	m_EntityOf[last] = INVALID_ENTITY_ID;

	--m_Size;

	return true;
}

template <typename T>
inline Component<T>::iterator Component<T>::begin() noexcept
{
	return std::addressof(m_Data[0].data);
}

template <typename T>
inline Component<T>::iterator Component<T>::end() noexcept
{
	return begin() + m_Size;
}

template <typename T>
inline Component<T>::const_iterator Component<T>::begin() const noexcept
{
	return std::addressof(m_Data[0].data);
}

template <typename T>
inline Component<T>::const_iterator Component<T>::end() const noexcept
{
	return begin() + m_Size;
}

template <typename T>
inline Component<T>::const_iterator Component<T>::cbegin() const noexcept
{
	return begin();
}

template <typename T>
inline Component<T>::const_iterator Component<T>::cend() const noexcept
{
	return end();
}

template <typename T>
inline Component<T>::reverse_iterator Component<T>::rbegin() noexcept
{
	return reverse_iterator(end());
}

template <typename T>
inline Component<T>::reverse_iterator Component<T>::rend() noexcept
{
	return reverse_iterator(begin());
}

template <typename T>
inline Component<T>::const_reverse_iterator Component<T>::rbegin() const noexcept
{
	return const_reverse_iterator(end());
}

template <typename T>
inline Component<T>::const_reverse_iterator Component<T>::rend() const noexcept
{
	return const_reverse_iterator(begin());
}

template <typename T>
inline Component<T>::const_reverse_iterator Component<T>::crbegin() const noexcept
{
	return rbegin();
}

template <typename T>
inline Component<T>::const_reverse_iterator Component<T>::crend() const noexcept
{
	return rend();
}


} // namespace Fleur::ECS
