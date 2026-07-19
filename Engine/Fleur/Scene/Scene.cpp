#include "Scene.h"

#include <Fleur/Math/Math.hpp>

#include "AssetsManager.h"
#include "FleurAllocator.hpp"
#include "Lux/Camera.h"
#include "Services/ServiceLocator.h"

namespace Fleur
{
Scene::Scene()
    : m_DirectionalLight(Fleur::Graphics::DirectionalLight(-Vec3(1.0f, 2.0f, 1.0f), Fleur::Graphics::Color::Red(), 1.0f))
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
    Mat4 transform = Fleur::Math::identity<Mat4>();
    auto assets = ServiceLocator::instance().GetService<AssetsManager>();

    m_SunTextureIdx = assets->LoadImage("DirectionalLightDebug.png").handle.id;
    m_FloorTextureIdx = assets->LoadImage("Floors/floor_stone_tile.png").handle.id;

    auto renderer = ServiceLocator::instance().GetService<Lux::Renderer>();
    renderer->CreateFloor(m_FloorTextureIdx, 0);

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


    // m_OmniLights.emplace_back(Vec3(1, 3, 0), 4, Fleur::Graphics::Color::Green(), 0.5);
}

void Scene::OnUpdate(float dtTime)
{
    if (m_Camera)
        m_Camera->OnUpdate(dtTime);

    // Debug
    auto assets = ServiceLocator::instance().GetService<AssetsManager>();
    auto renderer = ServiceLocator::instance().GetService<Lux::Renderer>();

    renderer->Debug().DrawAxes();

    float speed = 0.01f;
    Vec3 center = Vec3(0, 0, 0);
    Vec3 directionalDirection = m_DirectionalLight.GetDirection();

    // GetVirtualPosition() is the negated direction scaled by s_PosScale.
    // It must not be passed back into SetDirection(), otherwise the light
    // flips between two opposite positions every frame.
    m_DirectionalLight.SetDirection(Fleur::Math::RotatePointY(directionalDirection, center, dtTime * speed));
    m_DirectionalLight.DebugDraw(renderer.get(), m_SunTextureIdx);


    for (auto& instance : m_Instances)
    {
        auto* model = assets->Get<Fleur::Graphics::Model>(instance.model).obj;
        if (!model)
            return;

        const Vec3 targetCenter = Vec3(instance.transform[3]);
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
                    Mat4 transform = Mat4(transforms[srcInstance.transformStartIdx + j]);
                    transform = instance.transform * transform;

                    // transform[column][row]
                    Vec3 translation = Vec3(transform[3][0], transform[3][1], transform[3][2]);

                    Fleur::Graphics::BoundingBox boundingBox = srcPrimitive.GetBoundingBox();
                    renderer->Debug().BoundingBox(boundingBox, transform, Fleur::Graphics::Color::Magenta());
                    renderer->Debug().Point(translation + boundingBox.GetCenter(), Fleur::Graphics::Color::Green());
                }

                // Normals
                /*for (size_t k = 0; k < srcPrimitive.GetVertexCount(); k++)
                {
                    Mat4 modelMatrix = instance.transform * model->GetNodeTransforms()[srcInstance.transformStartIdx];
                    Mat3 normalMatrix =
                Fleur::Math::transpose(Fleur::Math::inverse(Mat3(modelMatrix)));
                    const auto& vertex =
                model->GetVerticesData()[srcPrimitive.GetVertexStart() + k];

                    Vec3 worldPos = Vec3(modelMatrix * Vec4(vertex.Position, 1.0));
                    Vec3 worldNormal = Fleur::Math::normalize(normalMatrix *
                vertex.Normal);

                    Vec3 lineStart = worldPos;
                    Vec3 lineEnd = worldPos + worldNormal * 0.1f;

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
            // instance.transform = Fleur::Math::rotate<float>(instance.transform, Fleur::Math::radians(0.5f), Vec3(0, 1, 0));
        }
        else
        {
            // instance.transform = Fleur::Math::rotate<float>(instance.transform, Fleur::Math::radians(-0.5f), Vec3(0, 1, 0));
        }
        renderer.Draw(instance.model.id, instance.transform);
        counter++;
    }
}

Fleur::Graphics::RenderFrameData Scene::GetFrameData() const
{
    return Fleur::Graphics::RenderFrameData{{
                                                m_Camera->GetView(),
                                                m_Camera->GetProjection(),
                                                m_Camera->GetCameraForward(),
                                                m_Camera->GetPosition(),
                                                m_Camera->NearClip(),
                                                m_Camera->FarClip(),
                                            },
                                            {.dirIntens{Vec4(m_DirectionalLight.GetDirection(), m_DirectionalLight.GetIntensity())},
                                             .color = m_DirectionalLight.GetColor().ToVec4(),
                                             .pos = Vec4(m_DirectionalLight.GetVirtualPosition(), 1)}};
}

}  // namespace Fleur
