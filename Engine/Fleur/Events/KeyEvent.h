#pragma once

#include "Event.h"
#include "KeyCodes.h"

namespace Fleur
{
template <typename Derived>
struct KeyEvent
{
    EKeyCode GetKeyCode() const
    {
        return m_KeyCode;
    }

protected:
    explicit KeyEvent(EKeyCode keyCode) noexcept
        : m_KeyCode(keyCode)
    {
    }
    EKeyCode m_KeyCode;
};

class FLEUR_API KeyPressedEvent final : public KeyEvent<KeyPressedEvent>, public EventBase<KeyPressedEvent>
{
public:
    KeyPressedEvent(EKeyCode keycode, int repeatCount) noexcept
        : EventBase(EVENT_NAME(EventBase))
        , KeyEvent(keycode)
        , m_RepeatCount(repeatCount)
    {
    }

    inline int GetRepeatCount() const
    {
        return m_RepeatCount;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "pressed key button: " << m_KeyCode << ", repeat count: " << m_RepeatCount;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<KeyPressedEvent>;
    int m_RepeatCount;
};

class FLEUR_API KeyReleasedEvent final : public KeyEvent<KeyReleasedEvent>, public EventBase<KeyReleasedEvent>
{
public:
    KeyReleasedEvent(EKeyCode keycode) noexcept
        : EventBase(EVENT_NAME(KeyReleasedEvent))
        , KeyEvent(keycode)
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "released key button: " << m_KeyCode;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<KeyReleasedEvent>;
};
}  // namespace Fleur
