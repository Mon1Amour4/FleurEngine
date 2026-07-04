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
    delete m_DirectionalLight;
    m_DirectionalLight = nullptr;
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
    // auto sponza = assets->LoadModelAsync("Sponza/Sponza2.glb");
    auto helmet = assets->LoadModelAsync("DamagedHelmet.glb");
    // auto box = assets->LoadModelAsync("Box.glb");
    //  auto free_stone_sphere = assets->LoadModelAsync("free_stone_sphere.glb");
    //  auto AlphaBlendModeTest = assets->LoadModelAsync("Sponza/AlphaBlendModeTest.glb");

    glm::mat4 transform = glm::identity<glm::mat4>();
    // m_Instances.push_back(SceneInstance{sponza->asset.handle, transform});
    m_Instances.push_back(SceneInstance{helmet->asset.handle, transform});

    // m_Instances.push_back(SceneInstance{box->asset.handle, transform});
    //  m_Instances.push_back(SceneInstance{free_stone_sphere->asset.handle, transform});
    //  m_Instances.push_back(SceneInstance{AlphaBlendModeTest->asset.handle, transform});

    m_DirectionalLight = new Fleur::Graphics::DirectionalLight(glm::vec3(-20, 0, 0), Fleur::Graphics::Color::Red(), 1);
    // m_OmniLights.emplace_back(glm::vec3(1, 3, 0), 4, Fleur::Graphics::Color::Green(), 0.5);
}

void Scene::OnUpdate(float dtTime)
{
    if (m_Camera)
        m_Camera->OnUpdate(dtTime);

    // Debug
    auto assets = ServiceLocator::instance().GetService<AssetsManager>();
    auto renderer = ServiceLocator::instance().GetService<Lux::Renderer>();

    renderer->Debug().DrawAxes();

    float speed = 1.f;
    static float angle = 0;
    angle += dtTime * speed;
    float radius = 10.0f;
    glm::vec3 center = glm::vec3(0, 0, 0);

    glm::vec3 pos;
    pos.x = center.x + cos(angle) * radius;
    pos.y = center.y;
    pos.z = center.z + sin(angle) * radius;

    // m_DirectionalLight->SetDirection(pos);

    for (auto& instance : m_Instances)
    {
        auto* model = assets->Get<Fleur::Graphics::Model>(instance.model).obj;
        if (!model)
            return;

        const glm::vec3 targetCenter = glm::vec3(instance.transform[3]);
        const float targetRadius = 2.0f;

        for (auto& pointLight : m_OmniLights)
        {
            const float distance = glm::length(pointLight.GetPosition() - targetCenter);

            if (distance > pointLight.GetRadius() + targetRadius)
                continue;

            pointLight.DebugDrawToTarget(renderer.get(), targetCenter, targetRadius, Fleur::Graphics::Color::Magenta());
        }

        m_DirectionalLight->DebugDrawToTarget(renderer.get(), instance.transform[3], 10, Fleur::Graphics::Color::Black());


        const auto* transforms = model->GetNodeTransforms();
        uint32_t instanceCount = model->GetMeshInstanceCount();
        const auto* meshes = model->GetMeshData();
        const auto* srcInstances = model->GetMeshInstanceData();
        for (size_t i = 0; i < instanceCount; i++)
        {
            const auto& srcInstance = srcInstances[i];
            const auto& mesh = meshes[srcInstance.meshIdx];

            const auto& srcPrimitives = mesh.GetPrimitives();
            for (size_t j = 0; j < mesh.GetPrimitiveCount(); j++)
            {
                const auto& srcPrimitive = srcPrimitives[j];
                if (srcPrimitive.GetAlphaMode() == Fleur::Graphics::FLAlphaMode::FL_BLEND)
                {
                    glm::mat4 transform = glm::mat4(transforms[srcInstance.transformStartIdx + j]);
                    transform = instance.transform * transform;

                    // transform[column][row]
                    glm::vec3 translation = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);

                    Fleur::Graphics::BoundingBox boundingBox = srcPrimitive.GetBoundingBox();
                    renderer->Debug().BoundingBox(boundingBox, transform, Fleur::Graphics::Color::Magenta());
                    renderer->Debug().Point(translation + boundingBox.GetCenter(), Fleur::Graphics::Color::Green());
                }

                // Normals
                /*for (size_t k = 0; k < srcPrimitive.GetVertexCount(); k++)
                {
                    glm::mat4 modelMatrix = instance.transform * model->GetNodeTransforms()[srcInstance.transformStartIdx];
                    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
                    const auto& vertex = model->GetVerticesData()[srcPrimitive.GetVertexStart() + k];

                    glm::vec3 worldPos = glm::vec3(modelMatrix * glm::vec4(vertex.Position, 1.0));
                    glm::vec3 worldNormal = glm::normalize(normalMatrix * vertex.Normal);

                    glm::vec3 lineStart = worldPos;
                    glm::vec3 lineEnd = worldPos + worldNormal * 0.1f;
                    renderer->Debug().Line(lineStart, lineEnd, Fleur::Graphics::Color::Black());
                }*/
            }
        }
    }

    // Lights
    if (m_DirectionalLight)
    {
        renderer->SetDirectionalLight(m_DirectionalLight->GetDirection(), m_DirectionalLight->GetColor(), m_DirectionalLight->GetIntensity());
    }
    renderer->UpdatePointLight(m_OmniLights.data(), m_OmniLights.size());
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
