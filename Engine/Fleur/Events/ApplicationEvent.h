#pragma once

#include "Event.h"

namespace Fleur
{

class FLEUR_API WindowResizeEvent : public EventBase<WindowResizeEvent>
{
public:
    WindowResizeEvent(float x, float y, float width, float height) noexcept
        : EventBase(EVENT_NAME(WindowResizeEvent))
        , m_X(x)
        , m_Y(y)
        , m_Width(width)
        , m_Height(height)
    {
    }

    inline float GetWidth() const
    {
        return m_Width;
    }
    inline float GetHeight() const
    {
        return m_Height;
    }
    inline float GetX() const
    {
        return m_X;
    }
    inline float GetY() const
    {
        return m_Y;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << " w: " << m_Width << ", h: " << m_Height << ", x: " << m_X << ", y:" << m_Y;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<WindowResizeEvent>;
    float m_X, m_Y, m_Width, m_Height;
};

class FLEUR_API WindowStartResizeEvent : public EventBase<WindowStartResizeEvent>
{
public:
    WindowStartResizeEvent(float x, float y, float width, float height, int cursorX, int cursorY) noexcept
        : EventBase(EVENT_NAME(WindowStartResizeEvent))
        , m_X(x)
        , m_Y(y)
        , m_Width(width)
        , m_Height(height)
        , m_CursorX(cursorX)
        , m_CursorY(cursorY)
    {
    }
    inline int Width() const
    {
        return m_Width;
    }
    inline int Height() const
    {
        return m_Height;
    }
    inline int X() const
    {
        return m_X;
    }
    inline int Y() const
    {
        return m_Y;
    }
    inline int CursorX() const
    {
        return m_CursorX;
    }
    inline int CursorY() const
    {
        return m_CursorY;
    }


protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    int m_X, m_Y, m_Width, m_Height, m_CursorX, m_CursorY;
    friend struct Fleur::EventBase<WindowStartResizeEvent>;
};

class FLEUR_API WindowEndResizeEvent : public EventBase<WindowEndResizeEvent>
{
public:
    WindowEndResizeEvent(float x, float y, float width, float height) noexcept
        : EventBase(EVENT_NAME(WindowEndResizeEvent))
    {
    }
    inline int Width() const
    {
        return m_Width;
    }
    inline int Height() const
    {
        return m_Height;
    }
    inline int X() const
    {
        return m_X;
    }
    inline int Y() const
    {
        return m_Y;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    int m_X, m_Y, m_Width, m_Height;
    friend struct Fleur::EventBase<WindowEndResizeEvent>;
};

class FLEUR_API WindowValidateEvent : public EventBase<WindowValidateEvent>
{
public:
    WindowValidateEvent() noexcept
        : EventBase(EVENT_NAME(WindowValidateEvent))
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<WindowValidateEvent>;
};

class FLEUR_API WindowCloseEvent : public EventBase<WindowCloseEvent>
{
public:
    WindowCloseEvent() noexcept
        : EventBase(EVENT_NAME(WindowCloseEvent))
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<WindowCloseEvent>;
};

class FLEUR_API AppTickEvent : public EventBase<AppTickEvent>
{
public:
    AppTickEvent() noexcept
        : EventBase(EVENT_NAME(AppTickEvent))
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<AppTickEvent>;
};

class FLEUR_API AppUpdateEvent : public EventBase<AppUpdateEvent>
{
public:
    AppUpdateEvent() noexcept
        : EventBase(EVENT_NAME(AppUpdateEvent))
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<AppUpdateEvent>;
};

class FLEUR_API AppRenderEvent : public EventBase<AppRenderEvent>
{
public:
    AppRenderEvent() noexcept
        : EventBase(EVENT_NAME(AppRenderEvent))
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<AppRenderEvent>;
};
}  // namespace Fleur
