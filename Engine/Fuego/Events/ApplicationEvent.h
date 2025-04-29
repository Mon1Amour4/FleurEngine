#pragma once

#include "Event.h"

namespace Fuego
{

class FUEGO_API WindowResizeEvent : public EventBase<WindowResizeEvent>
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
    friend struct Fuego::EventBase<WindowResizeEvent>;
    float x, y, _width, _height;
};

class FUEGO_API WindowStartResizeEvent : public EventBase<WindowStartResizeEvent>
{
public:
    WindowStartResizeEvent() noexcept
        : EventBase(EVENT_NAME(WindowStartResizeEvent))
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    friend struct Fuego::EventBase<WindowStartResizeEvent>;
};

class FUEGO_API WindowEndResizeEvent : public EventBase<WindowEndResizeEvent>
{
public:
    WindowEndResizeEvent() noexcept
        : EventBase(EVENT_NAME(WindowEndResizeEvent))
    {
    }

protected:
    std::string ToStringImpl() const
    {
        std::stringstream ss;
        return ss.str();
    }

private:
    friend struct Fuego::EventBase<WindowEndResizeEvent>;
};

class FUEGO_API WindowValidateEvent : public EventBase<WindowValidateEvent>
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
    friend struct Fuego::EventBase<WindowValidateEvent>;
};

class FUEGO_API WindowCloseEvent : public EventBase<WindowCloseEvent>
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
    friend struct Fuego::EventBase<WindowCloseEvent>;
};

class FUEGO_API AppTickEvent : public EventBase<AppTickEvent>
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
    friend struct Fuego::EventBase<AppTickEvent>;
};

class FUEGO_API AppUpdateEvent : public EventBase<AppUpdateEvent>
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
    friend struct Fuego::EventBase<AppUpdateEvent>;
};

class FUEGO_API AppRenderEvent : public EventBase<AppRenderEvent>
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
    friend struct Fuego::EventBase<AppRenderEvent>;
};
}  // namespace Fuego
