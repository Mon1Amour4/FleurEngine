#pragma once

#include <type_traits>

#include "Core.h"
#include "Services/ServiceInterfaces.hpp"

#define EVENT_NAME(type) (#type)

namespace Fleur
{
template <typename Derived>
struct EventBase
{
    std::string ToString() const
    {
        std::string derivedString(static_cast<const Derived*>(this)->ToStringImpl());
        std::stringstream ss;
        ss << m_EventName << derivedString;
        return ss.str();
    }
    bool GetHandled() const
    {
        return m_IsHandled;
    }
    void SetHandled()
    {
        m_IsHandled = true;
    }

protected:
    explicit EventBase(const char* name) noexcept
        : m_EventName(name)
        , m_IsHandled(false)
    {
    }
    const char* m_EventName;
    bool m_IsHandled;
};

}  // namespace Fleur
