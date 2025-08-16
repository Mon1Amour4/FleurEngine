#pragma once

#include "Event.h"
#include "KeyCodes.h"

namespace Fleur
{
template <typename Derived>
struct KeyEvent
{
    KeyCode GetKeyCode() const
    {
        return _keyCode;
    }

protected:
    explicit KeyEvent(KeyCode keycode) noexcept
        : _keyCode(keycode)
    {
    }
    KeyCode _keyCode;
};

class FLEUR_API KeyPressedEvent final : public KeyEvent<KeyPressedEvent>, public EventBase<KeyPressedEvent>
{
public:
    KeyPressedEvent(KeyCode keycode, int repeatCount) noexcept
        : EventBase(EVENT_NAME(EventBase))
        , KeyEvent(keycode)
        , _repeatCount(repeatCount)
    {
    }

    inline int GetRepeatCount() const
    {
        return _repeatCount;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "pressed key button: " << _keyCode << ", repeat count: " << _repeatCount;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<KeyPressedEvent>;
    int _repeatCount;
};

class FLEUR_API KeyReleasedEvent final : public KeyEvent<KeyReleasedEvent>, public EventBase<KeyReleasedEvent>
{
public:
    KeyReleasedEvent(KeyCode keycode) noexcept
        : EventBase(EVENT_NAME(KeyReleasedEvent))
        , KeyEvent(keycode)
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "released key button: " << _keyCode;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<KeyReleasedEvent>;
};
}  // namespace Fleur
