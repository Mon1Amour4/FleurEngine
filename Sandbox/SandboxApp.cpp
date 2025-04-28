#include "SandboxApp.h"

#include "Renderer.h"


SandboxApp::SandboxApp()
{
    SceneLayer* scene_layer = new SceneLayer();
    PushLayer(scene_layer);
}

SceneLayer::SceneLayer()
    : Layer("3D scene layer")
{
}

void SceneLayer::OnUpdate()
{
}

void SceneLayer::OnAttach()
{
    Fuego::FS::FileSystem& fs = Fuego::Application::instance().FileSystem();
}

void SceneLayer::OnDetach()
{
}

void SceneLayer::OnEvent(Fuego::EventVariant& event)
{
    // TODO: FU_CORE is not recognized here
    auto LogEventVisitor = Fuego::EventVisitor{[this](Fuego::AppRenderEvent& ev) { OnRenderEvent(ev); },
                                               [](const Fuego::EventVariant& ev) { /* FU_TRACE("{0}", ev.ToString());*/ }};

    std::visit(LogEventVisitor, event);
}

bool SceneLayer::OnRenderEvent(Fuego::AppRenderEvent& event)
{
    UNUSED(event);
    return true;
}
