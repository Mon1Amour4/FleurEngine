#pragma once

#include "Event.h"

namespace Fleur
{

class FLEUR_API WindowResizeEvent : public EventBase<WindowResizeEvent>
{
public:
    WindowResizeEvent(float x, float y, float width, float height) noexcept
        : EventBase(EVENT_NAME(WindowResizeEvent))
        , x(x)
        , y(y)
        , _width(width)
        , _height(height)
    {
    }

    inline float GetWidth() const
    {
        return _width;
    }
    inline float GetHeight() const
    {
        return _height;
    }
    inline float GetX() const
    {
        return x;
    }
    inline float GetY() const
    {
        return y;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        ss << " w: " << _width << ", h: " << _height << ", x: " << x << ", y:" << y;
        return ss.str();
    }

private:
    friend struct Fleur::EventBase<WindowResizeEvent>;
    float x, y, _width, _height;
};

class FLEUR_API WindowStartResizeEvent : public EventBase<WindowStartResizeEvent>
{
public:
    WindowStartResizeEvent(float x, float y, float width, float height, int cursorX, int cursorY) noexcept
        : EventBase(EVENT_NAME(WindowStartResizeEvent))
        , x(x)
        , y(y)
        , width(width)
        , height(height)
        , cursorX(cursorX)
        , cursorY(cursorY)
    {
    }
    inline int Width() const
    {
        return width;
    }
    inline int Height() const
    {
        return height;
    }
    inline int X() const
    {
        return x;
    }
    inline int Y() const
    {
        return y;
    }
    inline int CursorX() const
    {
        return cursorX;
    }
    inline int CursorY() const
    {
        return cursorY;
    }


protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    int x, y, width, height, cursorX, cursorY;
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
        return width;
    }
    inline int Height() const
    {
        return height;
    }
    inline int X() const
    {
        return x;
    }
    inline int Y() const
    {
        return y;
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    int x, y, width, height;
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
