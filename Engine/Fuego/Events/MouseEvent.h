#pragma once

#include <cstdint>

#include "Event.h"
#include "MouseCodes.h"

namespace Fuego
{
template <class Derived>
class MouseButtonEvent
{
public:
    int GetMouseButton() const
    {
        return _button;
    }

protected:
    explicit MouseButtonEvent(MouseCode button)
        : _button(button)
    {
    }

    uint16_t _button;
};

class FUEGO_API MouseMovedEvent final : public EventBase<MouseMovedEvent>
{
public:
    MouseMovedEvent(float x, float y)
        : EventBase(EVENT_NAME(MouseMovedEvent))
        , _mouseX(x)
        , _mouseY(y)
    {
    }

    inline float GetX() const
    {
        return _mouseX;
    }
    inline float GetY() const
    {
        return _mouseY;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "moved to x, y: " << _mouseX << " , " << _mouseY;
        return ss.str();
    }

private:
    friend struct Fuego::EventBase<MouseMovedEvent>;
    float _mouseX, _mouseY;
};

class FUEGO_API MouseScrolledEvent final : public EventBase<MouseScrolledEvent>
{
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : EventBase(EVENT_NAME(MouseScrolledEvent))
        , _offsetX(xOffset)
        , _offsetY(yOffset)
    {
    }

    inline float GetXOffset() const
    {
        return _offsetX;
    }
    inline float GetYOffset() const
    {
        return _offsetY;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "scrolled x, y: " << _offsetX << " , " << _offsetY;
        return ss.str();
    }

private:
    friend struct Fuego::EventBase<MouseScrolledEvent>;
    float _offsetX, _offsetY;
};

class FUEGO_API MouseButtonPressedEvent final : public EventBase<MouseButtonPressedEvent>, MouseButtonEvent<MouseButtonPressedEvent>
{
public:
    MouseButtonPressedEvent(MouseCode button)
        : EventBase(EVENT_NAME(MouseButtonPressedEvent))
        , MouseButtonEvent(button)
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "pressed button: " << _button;
        return ss.str();
    }

private:
    friend struct Fuego::EventBase<MouseButtonPressedEvent>;
};

class FUEGO_API MouseButtonReleasedEvent final : public EventBase<MouseButtonReleasedEvent>, MouseButtonEvent<MouseButtonReleasedEvent>
{
public:
    MouseButtonReleasedEvent(MouseCode button)
        : EventBase(EVENT_NAME(MouseButtonReleasedEvent))
        , MouseButtonEvent(button)
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "released button: " << _button;
        return ss.str();
    }

private:
    friend struct Fuego::EventBase<MouseButtonReleasedEvent>;
};
}  // namespace Fuego
