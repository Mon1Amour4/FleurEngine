#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>

#include "FleurAllocator.hpp"
#include "Renderer/Camera.h"

namespace Fleur
{
Scene::Scene() = default;

Scene::~Scene()
{
    if (m_Camera)
    {
        Fleur::Memory::FleurAllocator<Graphics::Camera> alloc;
        alloc.deallocate(m_Camera, 1);
        m_Camera = nullptr;
    }
}

void Scene::Init()
{
    Fleur::Memory::FleurAllocator<Graphics::Camera> alloc;
    m_Camera = alloc.construct_at();
    m_Camera->Activate();

    // Hardcoded scene. Model id is a placeholder until AssetsManager wiring
    // (Scene will request a load and store the returned AssetID here).
    glm::mat4 transform = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 100.0f)), glm::vec3(0.1f));
    m_Instances.push_back(SceneInstance{0, transform});
}

void Scene::Update(float dtTime)
{
    if (m_Camera)
        m_Camera->OnUpdate(dtTime);
}

void Scene::Submit(Lux::Renderer& renderer)
{
    for (const auto& instance : m_Instances)
        renderer.Draw(instance.model, instance.transform);
}

Lux::CameraView Scene::GetCamera() const
{
    return Lux::CameraView{m_Camera->GetView(), m_Camera->GetProjection(), m_Camera->GetCameraForward()};
}
}  // namespace Fleur
