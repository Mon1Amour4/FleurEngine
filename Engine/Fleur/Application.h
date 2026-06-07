#pragma once

#include <MemoryManager.h>

#include "EventQueue.h"
#include "Events/ApplicationEvent.h"
#include "Events/EventVisitor.h"
#include "FileSystem/FileSystem.h"
#include "FlTime.h"
#include "Layer.h"
#include "LayerStack.h"
#include "Renderer/Graphics.hpp"
#include "Services/ServiceLocator.h"
#include "Tessera.h"
#include "Window.h"
#include "singleton.hpp"

namespace Fleur::Graphics
{
class Model;
class Texture;
class Skybox;
}  // namespace Fleur::Graphics

namespace Fleur
{
class Scene;

class FLEUR_API Application : public singleton<Application>
{
    friend class singleton<Application>;

public:
    struct ApplicationBootSettings
    {
        Fleur::Graphics::EGraphicsAPI Renderer = Fleur::Graphics::EGraphicsAPI::Vulkan;
        bool Vsync = false;
        WindowProps WindowProperties = WindowProps{};
        float FixedDt = 0.025f;
    };

    void Init(ApplicationBootSettings& settings);

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
    bool OnMouseWheelScrollEvent(MouseScrolledEvent& event);

    Window& GetWindow();

    void SetVSync(bool active) const;
    bool IsVSync() const;

protected:
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<EventQueue> m_EventQueue;
    std::unique_ptr<Time> m_TimeManager;

    bool m_IsInitialized;
    bool m_IsRunning;
    LayerStack m_LayerStack;

    Application();
    virtual ~Application() override;

private:
    std::unique_ptr<Scene> m_Scene;
};

}  // namespace Fleur
