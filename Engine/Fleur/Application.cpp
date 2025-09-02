#include "Application.h"

#include "Events/EventVisitor.h"
#include "FileSystem/FileSystem.h"
#include "KeyCodes.h"
#include "Renderer.h"
#include "ThreadPool.h"
#include "Toolchain.h"

namespace Fleur
{
using Texture = Fleur::Graphics::Texture;
using Image2D = Fleur::Graphics::Image2D;
using CubemapImage = Fleur::Graphics::CubemapImage;
using Model = Fleur::Graphics::Model;
using Renderer = Fleur::Graphics::Renderer;
using Color = Fleur::Graphics::Color;

template <>
Application& singleton<Application>::instance()
{
    static Application inst;
    return inst;
}

Application::Application()
    : m_IsInitialized(false)
    , m_IsRunning(false)
{
}

Application::~Application()
{
}

void Application::PushLayer(Layer* layer)
{
    m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}
void Application::PushOverlay(Layer* overlay)
{
    m_LayerStack.PushOverlay(overlay);
}

void Application::OnEvent(EventVariant& event)
{  // clang-format off
    auto ApplicationEventVisitor = EventVisitor{[this](WindowResizeEvent&   ev) { OnWindowResize(ev); },
                                                [this](WindowStartResizeEvent&   ev) { OnStartResizeWindow(ev); },
                                                [this](WindowEndResizeEvent&    ev) {OnEndResizeWindow(ev); },
                                                [this](WindowValidateEvent&    ev) {OnValidateWindow(ev); },
                                                [this](WindowCloseEvent&    ev) {OnWindowClose(ev); },
                                                [this](AppRenderEvent&    ev) {OnRenderEvent(ev); },
                                                [this](KeyPressedEvent&    ev) {OnKeyPressEvent(ev); },
                                                [this](MouseScrolledEvent&    ev) {OnMouseWheelScrollEvent(ev); },
                                                [](auto&) {}
        };
    // clang-format on

    std::visit(ApplicationEventVisitor, event);

    for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
    {
        (*--it)->OnEvent(event);

        if (std::visit([](auto&& e) { return e.GetHandled(); }, event))
        {
            break;
        }
    }
}

bool Application::OnWindowClose(WindowCloseEvent& event)
{
    m_IsRunning = false;
    event.SetHandled();
    return true;
}
bool Application::OnWindowResize(WindowResizeEvent& event)
{
    ServiceLocator::instance().GetService<Renderer>()->UpdateViewport(event.GetX(), event.GetY(), event.GetWidth(), event.GetHeight());
    event.SetHandled();
    return true;
}
bool Application::OnStartResizeWindow(WindowStartResizeEvent& event)
{
    event.SetHandled();
    return true;
}
bool Application::OnEndResizeWindow(WindowEndResizeEvent& event)
{
    event.SetHandled();
    return true;
}
bool Application::OnValidateWindow(WindowValidateEvent& event)
{
    ServiceLocator::instance().GetService<Renderer>()->ValidateWindow();
    m_Window->SetPainted();
    event.SetHandled();
    return true;
}
bool Application::OnKeyPressEvent(KeyPressedEvent& event)
{
    EKeyCode crossplatformKey = event.GetKeyCode();

    switch (crossplatformKey)
    {
    case Key::D1:
        ServiceLocator::instance().GetService<Renderer>()->ToggleWireFrame();
        break;
    case Key::D2:
        m_Window->SwitchInteractionMode();
        break;
    }
    event.SetHandled();
    return true;
}

bool Application::OnRenderEvent(AppRenderEvent& event)
{
    auto renderer = ServiceLocator::instance().GetService<Renderer>();
    auto assetsManager = ServiceLocator::instance().GetService<Fleur::AssetsManager>();
    // renderer->ShowWireFrame();
    //  TODO: As for now we use just one opaque shader, but we must think about different passes
    //  using different shaders with blending and probably using pre-passes

    auto waterCoolerModel = assetsManager->Get<Model>("WaterCooler");
    auto waterCoolerModelLocked = waterCoolerModel.lock();
    if (waterCoolerModelLocked)
    {
        renderer->DrawModel(Fleur::Graphics::ERenderStage::STATIC_GEOMETRY, waterCoolerModelLocked.get(),
                            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 10.f)));
    }

    auto sponzaModel = assetsManager->Get<Model>("Sponza");
    auto sponzaModelLocked = sponzaModel.lock();
    if (sponzaModelLocked)
    {
        glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 100.f));
        glm::mat4 R = glm::mat4(1.f);
        glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(0.1f, 0.1f, 0.1f));
        glm::mat4 M = T * R * S;

        renderer->DrawModel(Fleur::Graphics::ERenderStage::STATIC_GEOMETRY, sponzaModelLocked.get(), M);
    }

    auto gizmoModel = assetsManager->Get<Model>("gizmo");
    if (!gizmoModel.expired())
    {
        glm::mat4 gizmoMatrix(1.0f);
        gizmoMatrix[3] = glm::vec4(-0.75f, -0.75f, 0.0f, 1.0f);

        renderer->DrawModel(Fleur::Graphics::ERenderStage::GIZMO, gizmoModel.lock().get(), gizmoMatrix);
    }

    UNUSED(event);
    return true;
}
bool Application::OnMouseMoveEvent(MouseMovedEvent& event)
{
    UNUSED(event);
    return true;
}

bool Application::OnMouseWheelScrollEvent(MouseScrolledEvent& event)
{
    m_Window->SetMouseWheelScrollData(event.GetXOffset(), event.GetYOffset());
    return false;
}

Window& Application::GetWindow()
{
    return *m_Window;
}

void Application::Init(ApplicationBootSettings& settings)
{
    m_EventQueue = EventQueue::CreateEventQueue();
    m_Window = Window::CreateAppWindow(settings.WindowProperties, *m_EventQueue);
    m_TimeManager = Time::CreateTimeManager(settings.FixedDt);

    auto fileSystem = ServiceLocator::instance().Register<Fleur::FS::FileSystem>();
    fileSystem.value()->Init();

    auto assetsManager = ServiceLocator::instance().Register<Fleur::AssetsManager>();
    auto renderer = ServiceLocator::instance().Register<Renderer>(Fleur::Graphics::EGraphicsAPI::OpenGL, std::make_unique<PostLoadToolchain>());
    renderer.value()->Init();
    renderer.value()->SetVSync(settings.Vsync);

    auto threadPool = ServiceLocator::instance().Register<Fleur::ThreadPool>();
    threadPool.value()->Init();


    auto resource = renderer.value()->CreateGraphicsResource<Texture>(assetsManager.value()->Load<Image2D>("fallback.png")->Resource());

    assetsManager.value()->Load<Model>("Sponza/Sponza.glb");
    assetsManager.value()->Load<Model>("gizmo.glb");
    assetsManager.value()->Load<Model>("WaterCooler/WaterCooler.obj");

    assetsManager.value()->Load<CubemapImage>("skybox.jpg");
    assetsManager.value()->Load<Image2D>("left.jpg");
    assetsManager.value()->Load<Image2D>("front.jpg");
    assetsManager.value()->Load<Image2D>("right.jpg");
    assetsManager.value()->Load<Image2D>("back.jpg");
    assetsManager.value()->Load<Image2D>("bottom.jpg");
    assetsManager.value()->Load<Image2D>("top.jpg");
    assetsManager.value()->Load<Image2D>("skybox_cubemap.jpg");  // cross-layour

    m_IsInitialized = true;
    m_IsRunning = true;
}

void Application::SetVSync(bool active) const
{
    auto renderer = ServiceLocator::instance().GetService<Renderer>();
    renderer->SetVSync(active);
}

bool Application::IsVSync() const
{
    auto renderer = ServiceLocator::instance().GetService<Renderer>();
    return renderer->IsVSync();
}

void Application::Run()
{
    if (!m_IsInitialized)
    {
        Application::ApplicationBootSettings settings{};
        Init(settings);
    }

    while (m_IsRunning)
    {
        auto renderer = ServiceLocator::instance().GetService<Renderer>();
        auto assetsManager = ServiceLocator::instance().GetService<Fleur::AssetsManager>();

        m_TimeManager->Tick();

        char buffer[32];
        sprintf_s(buffer, "%d", m_TimeManager->FPS());
        m_Window->SetTitle(buffer);

        float dtTime = m_TimeManager->DeltaTime();

        renderer->Clear();

        m_EventQueue->OnUpdate(dtTime);
        // TODO Do we need this lookup here or we need t move it to m_EventQueue->OnUpdate?
        while (!m_EventQueue->Empty())
        {
            auto ev = m_EventQueue->Front();
            OnEvent(*ev);
            m_EventQueue->Pop();
        }

        m_Window->OnUpdate(dtTime);
        Fleur::Graphics::Camera::GetActiveCamera()->OnUpdate(dtTime);

        for (auto layer : m_LayerStack)
        {
            layer->OnUpdate(dtTime);
        }

        renderer->OnUpdate(dtTime);
        renderer->Present();
        m_Window->SetMouseWheelScrollData(0, 0);
    }
}

}  // namespace Fleur
