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

void SceneLayer::OnUpdate(float dtTime)
{
}

void SceneLayer::OnAttach()
{
}

void SceneLayer::OnDetach()
{
}

void SceneLayer::OnEvent(Fleur::EventVariant& event)
{
    // TODO: FL_TRACE is not recognized here
    auto LogEventVisitor = Fleur::EventVisitor{[this](Fleur::AppRenderEvent& ev) { OnRenderEvent(ev); },
                                               [](const Fleur::EventVariant& ev) { /* FL_TRACE("{0}", ev.ToString());*/ }};

    std::visit(LogEventVisitor, event);
}

bool SceneLayer::OnRenderEvent(Fleur::AppRenderEvent& event)
{
    UNUSED(event);
    return true;
}
