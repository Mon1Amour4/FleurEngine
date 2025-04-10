#include "Application.h"
#include "Events/EventVisitor.h"
#include "FileSystem/FileSystem.h"
#include "KeyCodes.h"
#include "LayerStack.h"
#include "Renderer.h"
#include "Scene.h"

// Temporary TODO: remove
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#define ASSIMP_LOAD_FLAGS aiProcess_CalcTangentSpace \
| aiProcess_Triangulate \
| aiProcess_JoinIdenticalVertices \
| aiProcess_SortByPType \
// 

Fuego::Scene* scene;
std::unique_ptr<Fuego::Renderer::Texture> engine_texture;
int w, h, n;
unsigned char* texture_data;

namespace Fuego
{
class Application::ApplicationImpl
{
    friend class Application;
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<EventQueue> m_EventQueue;
    std::unique_ptr<Renderer::Renderer> _renderer;
    std::unique_ptr<Fuego::FS::FileSystem> _fs;
    std::vector<std::unique_ptr<Fuego::Renderer::Model>> _models;

    bool m_Running;
    LayerStack m_LayerStack;
    static Application* m_Instance;
};
Application* Application::ApplicationImpl::m_Instance = nullptr;
Application::Application()
    : d(new ApplicationImpl())
{
    ApplicationImpl::m_Instance = this;
    d->_fs = std::unique_ptr<Fuego::FS::FileSystem>(new Fuego::FS::FileSystem());
    d->m_EventQueue = EventQueue::CreateEventQueue();
    d->m_Window = Window::CreateAppWindow(WindowProps(), *d->m_EventQueue);
    d->_renderer.reset(new Renderer::Renderer());
    d->m_Running = true;
    d->_models.reserve(10);
    FS::FileSystem& fs = Application::Get().FileSystem();
    LoadModel("Model.obj");

    texture_data = fs.Load_Image("image.jpg", w, h, n);
    engine_texture = d->_renderer->CreateTexture(texture_data, w, h);

    scene = new Fuego::Scene("First scene");
}
Application::~Application()
{
    delete d;
    delete scene;
}

Renderer::Renderer& Application::Renderer()
{
    return *d->_renderer.get();
}

void Application::PushLayer(Layer* layer)
{
    d->m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}
void Application::PushOverlay(Layer* overlay)
{
    d->m_LayerStack.PushOverlay(overlay);
}

void Application::OnEvent(EventVariant& event)
{
    // clang-format off
    auto ApplicationEventVisitor = EventVisitor{[this](WindowCloseEvent&    ev) { OnWindowClose(ev); },
                                                [this](WindowResizeEvent&   ev) { OnWindowResize(ev); },
                                                [this](AppRenderEvent&      ev) { OnRenderEvent(ev); },
                                                [this](WindowStartResizeEvent&    ev) { OnStartResizeWindow(ev); },
                                                [this](WindowEndResizeEvent&    ev) { OnEndResizeWindow(ev); },
                                                [this](WindowValidateEvent&    ev) { OnValidateWindow(ev); },
                                                [this](MouseMovedEvent&    ev) { OnMouseMoveEvent(ev); },
                                                [this](KeyPressedEvent&    ev) { OnKeyPressEvent(ev); },
                                                [](Event&){}};
    // clang-format on

    std::visit(ApplicationEventVisitor, event);

    for (auto it = d->m_LayerStack.end(); it != d->m_LayerStack.begin();)
    {
        (*--it)->OnEvent(event);

        auto HandledEventVisitor = EventVisitor{[](const Event& ev) -> bool
                                                {
                                                    // FU_CORE_TRACE("{0}", ev.ToString());
                                                    return ev.GetHandled();
                                                }};

        if (std::visit(HandledEventVisitor, event))
        {
            break;
        }
    }
}

bool Application::OnWindowClose(WindowCloseEvent& event)
{
    d->m_Running = false;
    event.SetHandled();
    return true;
}
bool Application::OnWindowResize(WindowResizeEvent& event)
{
    d->_renderer->ChangeViewport(event.GetX(), event.GetY(), event.GetWidth(), event.GetHeight());
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
    d->_renderer->ValidateWindow();
    d->m_Window->SetPainted();
    event.SetHandled();
    return true;
}
bool Application::OnKeyPressEvent(KeyPressedEvent& event)
{
    KeyEvent& e = (KeyEvent&)event;
    KeyCode crossplatform_key = e.GetKeyCode();

    switch (crossplatform_key)
    {
    case Key::D1:
        d->_renderer->ToggleWireFrame();
        break;
    case Key::D2:
        d->m_Window->SwitchInteractionMode();
        break;
    }
    event.SetHandled();
    return true;
}
bool Application::OnRenderEvent(AppRenderEvent& event)
{
    d->_renderer->ShowWireFrame();
    Fuego::Renderer::Material* material = Fuego::Renderer::Material::CreateMaterial(engine_texture.get());

    //d->_renderer->DrawMesh(mesh_vector, engine_mesh->GetVertexCount(), material, glm::mat4(1.0f), Fuego::Renderer::Camera::GetActiveCamera()->GetView(),
                           //Fuego::Renderer::Camera::GetActiveCamera()->GetProjection());


    // event.SetHandled();
    UNUSED(event);
    return true;
}
bool Application::OnMouseMoveEvent(MouseMovedEvent& event)
{
    UNUSED(event);
    return true;
}

Application& Application::Get()
{
    return *Application::ApplicationImpl::m_Instance;
}
Fuego::FS::FileSystem& Application::FileSystem()
{
    return *d->_fs.get();
}
Window& Application::GetWindow()
{
    return *d->m_Window;
}

Fuego::Renderer::Model* Application::LoadModel(std::string_view path)
{
    Assimp::Importer importer{};
    const aiScene* scene = importer.ReadFile(d->_fs->GetFullPathToFile(path.data()), ASSIMP_LOAD_FLAGS);
    if (!scene)
        return nullptr;
    d->_models.emplace_back(std::make_unique<Fuego::Renderer::Model>(scene));
    Fuego::Renderer::Model* model = d->_models.back().get(); 
    
    return nullptr;
}

void Application::Run()
{
    while (d->m_Running)
    {
        d->_renderer->Clear();
        d->m_EventQueue->Update();
        d->m_Window->Update();
        Fuego::Renderer::Camera::GetActiveCamera()->Update();

        for (auto layer : d->m_LayerStack)
        {
            layer->OnUpdate();
        }

        while (!d->m_EventQueue->Empty())
        {
            auto ev = d->m_EventQueue->Front();
            OnEvent(*ev);
            d->m_EventQueue->Pop();
        }
        d->_renderer->Present();
    }
}


}  // namespace Fuego
