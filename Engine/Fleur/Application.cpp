#include "Application.h"

#include "Events/EventVisitor.h"
#include "FileSystem/FileSystem.h"
#include "KeyCodes.h"
#include "Lux/Lux.h"
#include "Lux/Toolchain.h"
#include "LightingSystem.h"
#include "Scene/Scene.h"
#include "ThreadPool.h"

#include <utility>

using Texture = Fleur::Graphics::Texture;
using Image2D = Fleur::Graphics::Image2D;
using CubemapImage = Fleur::Graphics::CubemapImage;
using Model = Fleur::Graphics::Model;
using Renderer = Lux::Renderer;
using Color = Fleur::Graphics::Color;

template <>
Fleur::Application& Fleur::singleton<Fleur::Application>::instance()
{
    static Application inst;
    return inst;
}

Fleur::Application::Application()
    : m_IsInitialized(false)
    , m_IsRunning(false)
{
}

Fleur::Application::~Application()
{
    m_LightingSystem.reset();
    delete m_WindowBarTitleBuffer;
}

void Fleur::Application::PushLayer(Layer* layer)
{
    m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}
void Fleur::Application::PushOverlay(Layer* overlay)
{
    m_LayerStack.PushOverlay(overlay);
}

void Fleur::Application::OnEvent(EventVariant& event)
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

bool Fleur::Application::OnWindowClose(WindowCloseEvent& event)
{
    m_IsRunning = false;
    event.SetHandled();

    return true;
}
bool Fleur::Application::OnWindowResize(WindowResizeEvent& event)
{
    event.SetHandled();
    return true;
}
bool Fleur::Application::OnStartResizeWindow(WindowStartResizeEvent& event)
{
    event.SetHandled();
    ServiceLocator::instance().GetService<Renderer>()->StartResize();
    return true;
}
bool Fleur::Application::OnEndResizeWindow(WindowEndResizeEvent& event)
{
    event.SetHandled();
    Fleur::SRect rect{event.X(), event.Y(), event.Width(), event.Height()};
    ServiceLocator::instance().GetService<Renderer>()->EndResize(rect);
    return true;
}
bool Fleur::Application::OnValidateWindow(WindowValidateEvent& event)
{
    m_Window->SetPainted();
    event.SetHandled();
    return true;
}
bool Fleur::Application::OnKeyPressEvent(KeyPressedEvent& event)
{
    EKeyCode crossplatformKey = event.GetKeyCode();

    switch (crossplatformKey)
    {
    case Key::D1:
        break;
    case Key::D2:
        m_Window->SwitchInteractionMode();
        break;
    case Key::F2:
        ServiceLocator::instance().GetService<Renderer>()->ToggleShadowMapPreview();
        break;
    case Key::D3:
    {
        auto renderer = ServiceLocator::instance().GetService<Renderer>();
        auto next = (renderer->GetBackendApi() == Fleur::Graphics::EGraphicsAPI::Vulkan) ? Fleur::Graphics::EGraphicsAPI::OpenGL
                                                                                         : Fleur::Graphics::EGraphicsAPI::Vulkan;
        renderer->SetBackend(next);
        break;
    }
    }
    event.SetHandled();
    return true;
}

bool Fleur::Application::OnRenderEvent(AppRenderEvent& event)
{
    return true;
}
bool Fleur::Application::OnMouseMoveEvent(MouseMovedEvent& event)
{
    UNUSED(event);
    return true;
}

bool Fleur::Application::OnMouseWheelScrollEvent(MouseScrolledEvent& event)
{
    m_Window->SetMouseWheelScrollData(event.GetXOffset(), event.GetYOffset());
    return false;
}

Fleur::Window& Fleur::Application::GetWindow()
{
    return *m_Window;
}

void Fleur::Application::Init(ApplicationBootSettings& settings)
{
    m_WindowBarTitleBuffer = new char[m_WindowBarBufferSize];

    Tessera::HelloTessera();

    Fleur::Memory::AllocAdapter::instance().Init(MM::MemoryManager::ManagerFabric(/*4096*/ 1024ULL * 1024ULL * 1024ULL * 2ULL));

    m_EventQueue = EventQueue::CreateEventQueue();
    m_Window = Fleur::Window::CreateAppWindow(settings.WindowProperties, *m_EventQueue);
    m_TimeManager = std::make_unique<Time>(settings.FixedDt);

    auto fileSystem = ServiceLocator::instance().Register<Fleur::FS::FileSystem>();
    fileSystem.value()->Init();

    auto threadPool = ServiceLocator::instance().Register<Fleur::ThreadPool>();
    threadPool.value()->Init();

    auto assetsManager = ServiceLocator::instance().Register<Fleur::AssetsManager>();
    assetsManager.value()->OnInit();

    assetsManager.value()->LoadImage("Placeholders/wall_placeholder2.png");

    auto renderer = ServiceLocator::instance().Register<Renderer>();
    renderer.value()->Init();
    renderer.value()->SetMaxPointLights(settings.maxPointLights);
    renderer.value()->SetBackend(settings.Renderer);
    renderer.value()->SetVSync(settings.Vsync);

    m_LightingSystem = std::make_unique<Fleur::Graphics::LightingSystem>(settings.maxPointLights);

    m_Scene = std::make_unique<Scene>(m_LightingSystem.get());
    m_Scene->Init();  // requests its own model loads via AssetsManager

    assetsManager->get()->LoadCubemapAsync(
        "skybox.jpg", {.sourceLayout = CUBEMAP_SOURCE_LAYOUT_EQUIRECTANGULAR_IMAGE},
        [](CubemapAsset asset)
        {
            auto renderer = ServiceLocator::instance().GetService<Renderer>();
            renderer->SetSkybox(asset.handle.id);
            FL_CORE_INFO("Skybox has uploaded to GPU");
        },
        Fleur::CallbackInvocationPoint::AFTER_GPU_UPLOAD);

    m_IsInitialized = true;
    m_IsRunning = true;
}

void Fleur::Application::SetVSync(bool active) const
{
    auto renderer = ServiceLocator::instance().GetService<Renderer>();
    renderer->SetVSync(active);
}

bool Fleur::Application::IsVSync() const
{
    auto renderer = ServiceLocator::instance().GetService<Renderer>();
    return renderer->IsVSync();
}

void Fleur::Application::Run()
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

        m_TimeManager->Tick(m_Window->IsActive());

        sprintf_s(m_WindowBarTitleBuffer, m_WindowBarBufferSize, "Fleur Engine %.2f %.3f\0", m_TimeManager->FPS(), m_TimeManager->DeltaTime());
        m_Window->SetTitle(m_WindowBarTitleBuffer);

        float dtTime = m_TimeManager->DeltaTime();

        m_EventQueue->OnUpdate(dtTime);
        // TODO Do we need this lookup here or we need t move it to m_EventQueue->OnUpdate?
        while (!m_EventQueue->Empty())
        {
            auto ev = m_EventQueue->Front();
            OnEvent(*ev);
            m_EventQueue->Pop();
        }

        m_Window->OnUpdate(dtTime);
        m_Scene->OnUpdate(dtTime);
        m_LightingSystem->Update(dtTime);

        for (auto layer : m_LayerStack)
        {
            layer->OnUpdate(dtTime);
        }

        assetsManager->OnUpdate(dtTime);

        Fleur::Graphics::RenderFrameData sceneFrameData = m_Scene->GetFrameData();
        Fleur::Graphics::LightingFrameData lightingFrameData = m_LightingSystem->ConsumeFrameData();
        sceneFrameData.directionalLight = lightingFrameData.directionalLight;
        sceneFrameData.pointLightsDirty = lightingFrameData.pointLightsDirty;
        if (lightingFrameData.pointLightsDirty)
        {
            sceneFrameData.pointLights = std::move(lightingFrameData.pointLights);
        }
        renderer->BeginFrame(sceneFrameData);
        m_Scene->Submit(*renderer);
        renderer->EndFrame();

        m_Window->SetMouseWheelScrollData(0, 0);
        // Fleur::Core::Benchmark::Frame();
    }

    // Release
    // TODO There should be Services lookup
    // Pseudocode
    // for(auto& services : Services)
    // {
    //      service.Shutdown();
    // }
    auto assetsManager = ServiceLocator::instance().GetService<Fleur::AssetsManager>();
    assetsManager->OnShutdown();

    auto renderer = ServiceLocator::instance().GetService<Renderer>();
    renderer->Shutdown();

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();
    threadPool->Shutdown();
}
