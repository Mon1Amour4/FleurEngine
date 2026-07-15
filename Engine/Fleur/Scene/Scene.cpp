#include "Scene.h"

#include <Fleur/Math/Math.hpp>

#include "AssetsManager.h"
#include "FleurAllocator.hpp"
#include "Lux/Camera.h"
#include "Services/ServiceLocator.h"

namespace Fleur
{
Scene::Scene()
    : m_DirectionalLight(Fleur::Graphics::DirectionalLight(-Fleur::Math::vec3(1.0f, 2.0f, 1.0f), Fleur::Graphics::Color::White(), 1.0f))
{
}
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
    Fleur::Math::mat4 transform = Fleur::Math::identity<Fleur::Math::mat4>();
    auto assets = ServiceLocator::instance().GetService<AssetsManager>();

    sunTextureIdx = assets->LoadImage("DirectionalLightDebug.png").handle.id;

    auto sponza = assets->LoadModelAsync("Sponza/Sponza2.glb");
    m_Instances.push_back(SceneInstance{sponza->asset.handle, transform});

    // auto helmet = assets->LoadModelAsync("DamagedHelmet.glb");
    // m_Instances.push_back(SceneInstance{helmet->asset.handle, transform});

    // auto box = assets->LoadModelAsync("Box.glb");
    // m_Instances.push_back(SceneInstance{box->asset.handle, transform});

    //  auto free_stone_sphere = assets->LoadModelAsync("free_stone_sphere.glb");
    //   m_Instances.push_back(SceneInstance{free_stone_sphere->asset.handle, transform});

    //  auto AlphaBlendModeTest = assets->LoadModelAsync("Sponza/AlphaBlendModeTest.glb");
    //   m_Instances.push_back(SceneInstance{AlphaBlendModeTest->asset.handle, transform});


    // m_OmniLights.emplace_back(Fleur::Math::vec3(1, 3, 0), 4, Fleur::Graphics::Color::Green(), 0.5);
}

void Scene::OnUpdate(float dtTime)
{
    if (m_Camera)
        m_Camera->OnUpdate(dtTime);

    // Debug
    auto assets = ServiceLocator::instance().GetService<AssetsManager>();
    auto renderer = ServiceLocator::instance().GetService<Lux::Renderer>();

    renderer->Debug().DrawAxes();

    float speed = 0.2f;
    static float angle = 0;
    angle += dtTime * speed;
    float radius = 100.0f;
    Fleur::Math::vec3 directionalPos = m_DirectionalLight.GetVirtualPosition();
    Fleur::Math::vec3 center = Fleur::Math::vec3(0, directionalPos.y, 0);

    Fleur::Math::vec3 pos;
    pos.x = center.x + cos(angle) * radius;
    pos.y = center.y;
    pos.z = center.z + sin(angle) * radius;


    m_DirectionalLight.SetDirection(pos);
    m_DirectionalLight.DebugDraw(renderer.get(), sunTextureIdx);


    for (auto& instance : m_Instances)
    {
        auto* model = assets->Get<Fleur::Graphics::Model>(instance.model).obj;
        if (!model)
            return;

        const Fleur::Math::vec3 targetCenter = Fleur::Math::vec3(instance.transform[3]);
        const float targetRadius = 2.0f;

        for (auto& pointLight : m_OmniLights)
        {
            const float distance = Fleur::Math::length(pointLight.GetPosition() - targetCenter);

            if (distance > pointLight.GetRadius() + targetRadius)
                continue;

            pointLight.DebugDrawToTarget(renderer.get(), targetCenter, targetRadius, Fleur::Graphics::Color::Magenta());
        }

        // m_DirectionalLight.DebugDrawToTarget(renderer.get(), instance.transform[3], 10, Fleur::Graphics::Color::Black());


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
                    Fleur::Math::mat4 transform = Fleur::Math::mat4(transforms[srcInstance.transformStartIdx + j]);
                    transform = instance.transform * transform;

                    // transform[column][row]
                    Fleur::Math::vec3 translation = Fleur::Math::vec3(transform[3][0], transform[3][1], transform[3][2]);

                    Fleur::Graphics::BoundingBox boundingBox = srcPrimitive.GetBoundingBox();
                    renderer->Debug().BoundingBox(boundingBox, transform, Fleur::Graphics::Color::Magenta());
                    renderer->Debug().Point(translation + boundingBox.GetCenter(), Fleur::Graphics::Color::Green());
                }

                // Normals
                /*for (size_t k = 0; k < srcPrimitive.GetVertexCount(); k++)
                {
                    Fleur::Math::mat4 modelMatrix = instance.transform * model->GetNodeTransforms()[srcInstance.transformStartIdx];
                    Fleur::Math::mat3 normalMatrix = Fleur::Math::transpose(Fleur::Math::inverse(Fleur::Math::mat3(modelMatrix)));
                    const auto& vertex = model->GetVerticesData()[srcPrimitive.GetVertexStart() + k];

                    Fleur::Math::vec3 worldPos = Fleur::Math::vec3(modelMatrix * Fleur::Math::vec4(vertex.Position, 1.0));
                    Fleur::Math::vec3 worldNormal = Fleur::Math::normalize(normalMatrix * vertex.Normal);

                    Fleur::Math::vec3 lineStart = worldPos;
                    Fleur::Math::vec3 lineEnd = worldPos + worldNormal * 0.1f;
                    renderer->Debug().Line(lineStart, lineEnd, Fleur::Graphics::Color::Black());
                }*/
            }
        }
    }

    // Lights
    renderer->UpdatePointLight(m_OmniLights.data(), m_OmniLights.size());
}

void Scene::Submit(Lux::Renderer& renderer)
{
    int counter = 0;
    for (auto& instance : m_Instances)
    {
        if (counter % 2 == 0)
        {
            // instance.transform = Fleur::Math::rotate<float>(instance.transform, Fleur::Math::radians(0.5f), Fleur::Math::vec3(0, 1, 0));
        }
        else
        {
            // instance.transform = Fleur::Math::rotate<float>(instance.transform, Fleur::Math::radians(-0.5f), Fleur::Math::vec3(0, 1, 0));
        }
        renderer.Draw(instance.model.id, instance.transform);
        counter++;
    }
}

Fleur::Graphics::RenderFrameData Scene::GetFrameData() const
{
    return Fleur::Graphics::RenderFrameData{{m_Camera->GetCameraForward(), m_Camera->GetView(), m_Camera->GetProjection()},
                                            {.dirIntens{Fleur::Math::vec4(m_DirectionalLight.GetDirection(), m_DirectionalLight.GetIntensity())},
                                             .color = m_DirectionalLight.GetColor().ToVec4(),
                                             .pos = Fleur::Math::vec4(m_DirectionalLight.GetVirtualPosition(), 1)}};
}

}  // namespace Fleur
