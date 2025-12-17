#pragma once

#include "EventQueue.h"
#include "Events/ApplicationEvent.h"
#include "Events/EventVisitor.h"
#include "FileSystem/FileSystem.h"
#include "FlTime.h"
#include "Graphics.hpp"
#include "Layer.h"
#include "LayerStack.h"
#include "Services/ServiceLocator.h"
#include "Window.h"
#include "singleton.hpp"

namespace Fleur::Graphics
{
class Renderer;
class Model;
class Texture;
}  // namespace Fleur::Graphics

namespace Fleur
{

class FLEUR_API FleurAllocator : public singleton<FleurAllocator>
{
    friend class singleton<FleurAllocator>;

    MM::MemoryManager* memory;

public:
    void Init(MM::MemoryManager* manager)
    {
        if (manager)
            memory = manager;
    }

    template <class T, size_t Align = 0>
    [[nodiscard]] T* allocate(uint32_t count = 1)
    {
        return memory->allocate<T, Align>(count);
    }

    template <class T>
    void deallocate(void* ptr, uint32_t count)
    {
        memory->deallocate<int>(ptr, count);
    }

    template <typename T, size_t Align = 0, typename... Args>
    [[nodiscard]] T* construct_at(Args&&... args)
    {
        return memory->construct_at<T, Align>(std::forward<Args>(args)...);
    }

    template <typename T, size_t Align = 0, typename... Args>
    [[nodiscard]] T* construct_array_at(uint32_t count, Args&&... args)
    {
        return memory->construct_array_at<T, Align>(count, std::forward<Args>(args)...);
    }
};

class FLEUR_API Application : public singleton<Application>
{
    friend class singleton<Application>;

public:
    struct ApplicationBootSettings
    {
        Fleur::Graphics::EGraphicsAPI Renderer = Fleur::Graphics::EGraphicsAPI::OpenGL;
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
};
}  // namespace Fleur
