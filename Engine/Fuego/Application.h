#pragma once

#include "EventQueue.h"
#include "Events/ApplicationEvent.h"
#include "Events/EventVisitor.h"
#include "FileSystem/FileSystem.h"
#include "Layer.h"
#include "Window.h"
#include "singleton.hpp"


namespace Fuego::Renderer
{
class Renderer;
class FileSystem;
}  // namespace Fuego::Renderer

namespace Fuego
{

class FUEGO_API Application : public singleton<Application>
{
    friend class singleton<Application>;

    FUEGO_INTERFACE(Application)

public:
    void Init();
    void Run();

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

    void OnEvent(EventVariant& event);
    bool OnRenderEvent(AppRenderEvent& event);

    // Window events:
    bool OnWindowClose(WindowCloseEvent& event);
    bool OnWindowResize(WindowResizeEvent& event);
    bool OnStartResizeWindow(WindowStartResizeEvent& event);
    bool OnEndResizeWindow(WindowEndResizeEvent& event);
    bool OnValidateWindow(WindowValidateEvent& event);
    bool OnKeyPressEvent(KeyPressedEvent& event);

    // Input events
    bool OnMouseMoveEvent(MouseMovedEvent& event);

    Fuego::Renderer::Renderer& Renderer();
    Fuego::FS::FileSystem& FileSystem();
    Window& GetWindow();

protected:
    Application();
    virtual ~Application() override;
};
}  // namespace Fuego
