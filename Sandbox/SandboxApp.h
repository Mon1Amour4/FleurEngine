#pragma once

#include <Fleur.h>

class SceneLayer final : public Fleur::Layer
{
public:
    SceneLayer();

    virtual void OnUpdate(float dtTime) override;
    virtual void OnAttach() override;
    virtual void OnDetach() override;

    virtual void OnEvent(Fleur::EventVariant& event) override;
    bool OnRenderEvent(Fleur::AppRenderEvent& event);
};

class SandboxApp : public Fleur::Application
{
    SandboxApp();
};
