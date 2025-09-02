#pragma once

#include "Event.h"
#include "MouseCodes.h"

namespace Fleur
{
template <class Derived>
class MouseButtonEvent
{
public:
    int GetMouseButton() const
    {
        return m_Button;
    }

protected:
    explicit MouseButtonEvent(EMouseCode button) noexcept
        : m_Button(button)
    {
    }

    uint16_t m_Button;
};

class FLEUR_API MouseMovedEvent final : public EventBase<MouseMovedEvent>
{
public:
    MouseMovedEvent(uint32_t x, uint32_t y) noexcept
        : EventBase(EVENT_NAME(MouseMovedEvent))
        , m_MouseX(x)
        , m_MouseY(y)
    {
    }

    inline uint32_t GetX() const
    {
        return m_MouseX;
    }
    inline uint32_t GetY() const
    {
        return m_MouseY;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "moved to x, y: " << m_MouseX << " , " << m_MouseY;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<MouseMovedEvent>;
    uint32_t m_MouseX, m_MouseY;
};

class FLEUR_API MouseScrolledEvent final : public EventBase<MouseScrolledEvent>
{
public:
    MouseScrolledEvent(int xOffset, int yOffset) noexcept
        : EventBase(EVENT_NAME(MouseScrolledEvent))
        , m_OffsetX(xOffset)
        , m_OffsetY(yOffset)
    {
    }

    inline int GetXOffset() const
    {
        return m_OffsetX;
    }
    inline int GetYOffset() const
    {
        return m_OffsetY;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "scrolled x, y: " << m_OffsetX << " , " << m_OffsetY;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<MouseScrolledEvent>;
    int m_OffsetX, m_OffsetY;
};

class FLEUR_API MouseButtonPressedEvent final : public EventBase<MouseButtonPressedEvent>, MouseButtonEvent<MouseButtonPressedEvent>
{
public:
    MouseButtonPressedEvent(EMouseCode button) noexcept
        : EventBase(EVENT_NAME(MouseButtonPressedEvent))
        , MouseButtonEvent(button)
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "pressed button: " << m_Button;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<MouseButtonPressedEvent>;
};

class FLEUR_API MouseButtonReleasedEvent final : public EventBase<MouseButtonReleasedEvent>, MouseButtonEvent<MouseButtonReleasedEvent>
{
public:
    MouseButtonReleasedEvent(EMouseCode button) noexcept
        : EventBase(EVENT_NAME(MouseButtonReleasedEvent))
        , MouseButtonEvent(button)
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << "released button: " << m_Button;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<MouseButtonReleasedEvent>;
};
}  // namespace Fleur
