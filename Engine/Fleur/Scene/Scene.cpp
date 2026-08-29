#include "Scene.h"

#include <Fleur/Math/Math.hpp>
#include <algorithm>
#include <cmath>

#include "AssetsManager.h"
#include "FleurAllocator.hpp"
#include "Lux/Camera.h"
#include "Services/ServiceLocator.h"

namespace Fleur
{
Scene::Scene(Fleur::Graphics::LightingSystem* lightingSystem)
    : m_LightingSystem(lightingSystem)
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

    m_FloorTextureIdx = assets->LoadImage("Floors/floor_stone_tile.png").handle.id;
    m_SunTextureIdx = assets->LoadImage("DirectionalLightDebug.png").handle.id;

    m_DirectionalLightHandle = m_LightingSystem->CreateDirectionalLight(Vec3(-1.0f, -2.0f, 1.0f), Fleur::Graphics::Color::White(), 0.7f);
    m_PointLightHandles.emplace_back(m_LightingSystem->CreatePointLight(Vec3(8, 2, +2), 5, Fleur::Graphics::Color::Red(), 0.7f));
    m_PointLightHandles.emplace_back(m_LightingSystem->CreatePointLight(Vec3(-9, 2, -2), 5, Fleur::Graphics::Color::Green(), 0.7f));
    m_PointLightHandles.emplace_back(m_LightingSystem->CreatePointLight(Vec3(0, 1, 0), 3, Fleur::Graphics::Color::White(), 0.7f));

    // Deferred stress scene: 100 total lights, each with a cubemap shadow.
    constexpr uint32_t kStressPointLightCount = 10;
    m_PointLightHandles.reserve(kStressPointLightCount);
    const std::array<Fleur::Graphics::Color, 3> stressColors{
        Fleur::Graphics::Color::Red(),
        Fleur::Graphics::Color::Green(),
        Fleur::Graphics::Color::White(),
    };
    for (uint32_t lightIndex = static_cast<uint32_t>(m_PointLightHandles.size()); lightIndex < kStressPointLightCount; ++lightIndex)
    {
        const uint32_t gridX = lightIndex % 25;
        const uint32_t gridZ = (lightIndex / 25) % 20;
        const uint32_t gridY = (lightIndex / (25 * 20)) % 2;
        const Vec3 position{(static_cast<float>(gridX) - 12.0f) * 1.5f, 1.0f + static_cast<float>(gridY) * 3.0f, (static_cast<float>(gridZ) - 9.5f) * 1.5f};
        m_PointLightHandles.emplace_back(m_LightingSystem->CreatePointLight(position, 4.0f, stressColors[lightIndex % stressColors.size()], 0.08f));
    }

    auto renderer = ServiceLocator::instance().GetService<Lux::Renderer>();
    renderer->CreateFloor(m_FloorTextureIdx, 0);

    auto sponza = assets->LoadModelAsync("Sponza/Fleur_Scene_ai_test.glb");
    m_Instances.push_back(SceneInstance{sponza->asset.handle, transform});

    // auto helmet = assets->LoadModelAsync("DamagedHelmet.glb");
    // m_Instances.push_back(SceneInstance{helmet->asset.handle, transform});

    // auto box = assets->LoadModelAsync("Box.glb");
    // m_Instances.push_back(SceneInstance{box->asset.handle, transform});

    //  auto free_stone_sphere = assets->LoadModelAsync("free_stone_sphere.glb");
    //   m_Instances.push_back(SceneInstance{free_stone_sphere->asset.handle, transform});

    //  auto AlphaBlendModeTest = assets->LoadModelAsync("Sponza/AlphaBlendModeTest.glb");
    //   m_Instances.push_back(SceneInstance{AlphaBlendModeTest->asset.handle, transform});
}

void Scene::OnUpdate(float dtTime)
{
    if (m_Camera)
        m_Camera->OnUpdate(dtTime);

    // Debug
    auto assets = ServiceLocator::instance().GetService<AssetsManager>();
    auto renderer = ServiceLocator::instance().GetService<Lux::Renderer>();

    // renderer->Debug().DrawAxes();

    float speed = 0.08f;
    Vec3 center = Vec3(0, 0, 0);
    const auto* directionalLight = m_LightingSystem->GetDirectionalLight(m_DirectionalLightHandle);
    if (!directionalLight)
        return;

    Vec3 directionalDirection = directionalLight->GetDirection();

    // GetVirtualPosition() is the negated direction scaled by s_PosScale.
    // It must not be passed back into SetDirection(), otherwise the light
    // flips between two opposite positions every frame.
    m_LightingSystem->SetDirection(m_DirectionalLightHandle, Fleur::Math::RotatePointY(directionalDirection, center, dtTime * speed));

    m_LightingSystem->GetDirectionalLight(m_DirectionalLightHandle)->DebugDraw(&*renderer, m_SunTextureIdx);

    // Keep the stress lights moving without creating or destroying lights.
    // The first three are the original scene lights; all later lights orbit
    // around their grid cells with individual phases.
    m_StressLightTime += dtTime;
    constexpr size_t kFirstStressLightIndex = 3;
    /*for (size_t lightIndex = kFirstStressLightIndex; lightIndex < m_PointLightHandles.size(); ++lightIndex)
    {
        const uint32_t stressIndex =
     * static_cast<uint32_t>(lightIndex - kFirstStressLightIndex);
        const uint32_t gridX = stressIndex % 25;
        const uint32_t gridZ = (stressIndex
     * / 25) % 20;
        const uint32_t gridY = (stressIndex / (25 * 20)) % 2;
        const float phase = static_cast<float>(stressIndex) * 0.173f;
 const
     * float angularSpeed = 0.6f + static_cast<float>(stressIndex % 5) * 0.08f;
        const float angle = m_StressLightTime * angularSpeed + phase;
 const
     * Vec3 position{(static_cast<float>(gridX) - 12.0f) * 1.5f + std::cos(angle) * 0.75f,
                            1.0f + static_cast<float>(gridY) * 3.0f +
     * std::sin(angle * 1.7f) * 0.5f,
                            (static_cast<float>(gridZ) - 9.5f) * 1.5f + std::sin(angle) * 0.75f};

     * m_LightingSystem->SetPosition(m_PointLightHandles[lightIndex], position);
    }*/

    const auto lightingFrameData = m_LightingSystem->BuildFrameData();
    for (const auto& pointLight : lightingFrameData.pointLights)
    {
        const Fleur::Graphics::Color lightColor(pointLight.color.x, pointLight.color.y, pointLight.color.z);
        // renderer->Debug().Sphere(pointLight.pos, pointLight.radius, lightColor);

        const float iconSize = std::max(0.1f, pointLight.radius * 0.1f);
        // renderer->Debug().Billboard(pointLight.pos, Fleur::Vec2(iconSize), m_SunTextureIdx, false);
    }


    for (auto& instance : m_Instances)
    {
        auto* model = assets->Get<Fleur::Graphics::Model>(instance.model).obj;
        // The cache exposes the Model object before async loading finishes.
        // Do not consume its default (infinite) bounding box or empty GPU data.
        if (!model || model->GetMeshInstanceCount() == 0 || model->GetPrimitiveCount() == 0)
            continue;

        if (!instance.shadowBoundsRegistered)
        {
            renderer->RegisterShadowInstance(instance.model.id, instance.transform, model->GetBoundingBox());
            instance.shadowBoundsRegistered = true;
        }

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
                    // renderer->Debug().BoundingBox(boundingBox, transform, Fleur::Graphics::Color::Magenta());
                    // renderer->Debug().Point(translation + boundingBox.GetCenter(), Fleur::Graphics::Color::Green());
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

Fleur::Graphics::RenderFrameData Scene::GetFrameData()
{
    Fleur::Graphics::RenderFrameData frameData{
        .camera =
            {
                m_Camera->GetView(),
                m_Camera->GetProjection(),
                m_Camera->GetCameraForward(),
                m_Camera->GetPosition(),
                m_Camera->NearClip(),
                m_Camera->FarClip(),
            },
    };
    return frameData;
}

}  // namespace Fleur
