#include "SandboxApp.h"

#include "Renderer.h"

Fuego::Application* Fuego::CreateApplication()
{
    return new SandboxApp();
}

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
    Fuego::FS::FileSystem& fs = Fuego::Application::Get().FileSystem();
}

void SceneLayer::OnDetach()
{
}

void SceneLayer::OnEvent(Fuego::EventVariant& event)
{
    auto LogEventVisitor =
        Fuego::EventVisitor{[this](Fuego::AppRenderEvent& ev) { OnRenderEvent(ev); }, [](const Fuego::Event& ev) { /*FU_TRACE("{0}", ev.ToString());*/ }};

    std::visit(LogEventVisitor, event);
}

bool SceneLayer::OnRenderEvent(Fuego::AppRenderEvent& event)
{
    glm::mat4 model_pos = glm::translate(glm::mat4(1.0f), glm::vec3(15.0f, 0.0f, -5.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0F / 720.0F, 0.1f, 100.0f);

    //Fuego::Renderer::Material* material = Fuego::Renderer::Material::CreateMaterial(sandbox_texture.get());
    //Fuego::Renderer::Renderer& renderer = Fuego::Application::Get().Renderer();
    /*renderer.DrawMesh(sandbox_mesh_vector, sandbox_mesh->GetVertexCount(), material, glm::mat4(1.0f), Fuego::Renderer::Camera::GetActiveCamera()->GetView(),
                      Fuego::Renderer::Camera::GetActiveCamera()->GetProjection());*/
    UNUSED(event);
    return true;
}
