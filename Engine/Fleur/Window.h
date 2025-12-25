#pragma once

#include "EventQueue.h"

namespace Fleur
{

enum EWindowMode
{
    MINIMIZED = 0,
    MAXIMIZED = 1,
    RESTORED = 2,
};

struct WindowProps
{
    std::string Title = "Fleur Engine";
    long x = 100;
    long y = 100;
    uint32_t Width = 1280;
    uint32_t Height = 720;
    EWindowMode mode = RESTORED;

    bool Centered = true;
    bool Resizable = true;
    bool Movable = true;
    bool Closable = true;
    bool Minimizable = true;
    bool Maximizable = true;
    bool CanFullscreen = true;

    uint32_t BackgroundColor = 0xFFFFFFFF;
    bool Transparent = false;
    bool Frame = true;
    bool HasShadow = true;

#ifdef FLEUR_PLATFORM_WIN
    const TCHAR* APP_WINDOW_CLASS_NAME = TEXT("MainAppWindow Engine");
#endif
};

class Surface;

class Window : public IUpdatable
{
    friend class Application;

public:
    enum EInteractionMode
    {
        EDITOR = 0,
        GAMING = 1
    };
    virtual ~Window() = default;

    virtual void OnUpdate(float dtTime) = 0;
    virtual void OnPostUpdate(float dtTime) = 0;
    virtual void OnFixedUpdate() = 0;

    virtual unsigned int GetWidth() const = 0;
    virtual unsigned int GetHeight() const = 0;

    virtual bool IsActive() const = 0;

    virtual void* GetNativeHandle() const = 0;

    virtual inline bool IsResizing() const = 0;

    inline virtual glm::vec2 GetMouseDir() const = 0;
    virtual bool HasMouseMoved(int x, int y) const = 0;

    virtual EInteractionMode GetInteractionMode() const = 0;
    virtual void SwitchInteractionMode() = 0;

    virtual void SetTitle(std::string title) = 0;

    static std::unique_ptr<Window> CreateAppWindow(const WindowProps& props, EventQueue& eventQueue);

private:
    virtual void SetMousePos(int x, int y) = 0;
    virtual void SetPainted() = 0;
    virtual void SetMouseWheelScrollData(int x, int y) = 0;
};
}  // namespace Fleur
