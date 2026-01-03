#include "SandboxApp.h"

#include "Renderer/Renderer.h"

SandboxApp::SandboxApp()
{
    SceneLayer* sceneLayer = new SceneLayer();
    PushLayer(sceneLayer);
}

SceneLayer::SceneLayer()
    : Layer("3D scene layer")
{
}

void SceneLayer::OnUpdate(float dtTime)
{
    UNUSED(dtTime);
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
    auto logEventVisitor = Fleur::EventVisitor{[this](Fleur::AppRenderEvent& ev) { OnRenderEvent(ev); },
                                               [](const Fleur::EventVariant& ev)
                                               {
                                                   UNUSED(ev); /* FL_TRACE("{0}", ev.ToString());*/
                                               }};

    std::visit(logEventVisitor, event);
}

bool SceneLayer::OnRenderEvent(Fleur::AppRenderEvent& event)
{
    UNUSED(event);
    return true;
}
