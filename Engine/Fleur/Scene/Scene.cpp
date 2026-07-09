#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>

#include "AssetsManager.h"
#include "FleurAllocator.hpp"
#include "Lux/Camera.h"
#include "Services/ServiceLocator.h"

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

    // Hardcoded scene. Request the model load and store the AssetID the async op
    // hands back synchronously (the id is assigned at registration, before the
    // load finishes); the renderer registers geometry under the same id later.
    auto assets = ServiceLocator::instance().GetService<AssetsManager>();
    auto sponza = assets->LoadModelAsync("Sponza/Sponza2.glb");
    auto AlphaBlendModeTest = assets->LoadModelAsync("Sponza/AlphaBlendModeTest.glb");

    glm::mat4 transform = glm::identity<glm::mat4>();
    m_Instances.push_back(SceneInstance{sponza->asset.handle, transform});
    m_Instances.push_back(SceneInstance{AlphaBlendModeTest->asset.handle, transform});
}

void Scene::OnUpdate(float dtTime)
{
    if (m_Camera)
        m_Camera->OnUpdate(dtTime);

    // Debug
    auto assets = ServiceLocator::instance().GetService<AssetsManager>();

    for (auto& ins : m_Instances)
    {
        auto* model = assets->Get<Fleur::Graphics::Model>(ins.model).obj;
        if (model)
        {
            auto renderer = ServiceLocator::instance().GetService<Lux::Renderer>();
            uint32_t meshCount = model->GetMeshCount();
            const auto& meshes = model->GetMeshData();
            for (int i = 0; i < meshCount; ++i)
            {
                const auto& mesh = meshes[i];
                for (size_t j = 0; j < mesh.GetPrimitiveCount(); j++)
                {
                    if (mesh.GetPrimitives()[j].GetAlphaMode() == Fleur::Graphics::FLAlphaMode::FL_BLEND)
                    {
                        Fleur::Graphics::SAABB aabb = mesh.GetAABB();
                        renderer->Debug().AABB(aabb, glm::mat4(1.f), Fleur::Graphics::Color::Magenta());
                        renderer->Debug().Point(aabb.center, Fleur::Graphics::Color::Green());
                    }
                }
            }
        }
    }
}

void Scene::Submit(Lux::Renderer& renderer)
{
    int counter = 0;
    for (auto& instance : m_Instances)
    {
        if (counter % 2 == 0)
        {
            // instance.transform = glm::rotate<float>(instance.transform, glm::radians(0.5f), glm::vec3(0, 1, 0));
        }
        else
        {
            // instance.transform = glm::rotate<float>(instance.transform, glm::radians(-0.5f), glm::vec3(0, 1, 0));
        }
        renderer.Draw(instance.model.id, instance.transform);
        counter++;
    }
}

Lux::CameraView Scene::GetCamera() const
{
    return Lux::CameraView{m_Camera->GetView(), m_Camera->GetProjection(), m_Camera->GetCameraForward()};
}

}  // namespace Fleur
