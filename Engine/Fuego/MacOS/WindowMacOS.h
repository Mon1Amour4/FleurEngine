#pragma once

#include "Core.h"
#include "Window.h"

namespace Fuego
{
class EventQueueMacOS;

class WindowMacOS : public Window
{
public:
    WindowMacOS(const WindowProps& props, EventQueue& eventQueue);
    ~WindowMacOS();

    virtual void Update() override;

    inline virtual unsigned int GetWidth() const override
    {
        return _props.Width;
    }
    inline virtual unsigned int GetHeight() const override
    {
        return _props.Height;
    }

    inline virtual void SetVSync(bool enabled) override
    {
        UNUSED(enabled);
    }
    inline virtual bool IsVSync() const override
    {
        return true;
    }

    virtual const void* GetNativeHandle() const override;

private:
    void Init(const WindowProps& props, EventQueue& eventQueue);
    void Shutdown();

    void* _window;
    void* _view;

    WindowProps _props;
    EventQueueMacOS* _eventQueue;
};
}  // namespace Fuego
