#pragma once

#include <Fuego.h>

namespace Fuego::Renderer
{
    class Model;
}

class SceneLayer final : public Fuego::Layer
{
public:
    SceneLayer();

    virtual void OnUpdate() override;
    virtual void OnAttach() override;
    virtual void OnDetach() override;

    virtual void OnEvent(Fuego::EventVariant& event) override;
    bool OnRenderEvent(Fuego::AppRenderEvent& event);

private:
    std::vector<std::unique_ptr<Fuego::Renderer::Model>> scene_meshes;
    std::vector<std::vector<float>> mesh_data;
};

class SandboxApp : public Fuego::Application
{
public:
    SandboxApp();
};
