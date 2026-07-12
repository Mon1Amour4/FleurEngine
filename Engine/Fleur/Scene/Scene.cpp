#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>

#include "AssetsManager.h"
#include "FleurAllocator.hpp"
#include "Lux/Camera.h"
#include "Services/ServiceLocator.h"

namespace Fleur
{
Scene::Scene()
    : m_DirectionalLight(Fleur::Graphics::DirectionalLight(glm::vec3(-1, 0, 0), Fleur::Graphics::Color::Red(), 1))
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
    glm::mat4 transform = glm::identity<glm::mat4>();
    auto assets = ServiceLocator::instance().GetService<AssetsManager>();

    sunTextureIdx = assets->LoadImage("DirectionalLightDebug.png").handle.id;

    // auto sponza = assets->LoadModelAsync("Sponza/Sponza2.glb");
    // m_Instances.push_back(SceneInstance{sponza->asset.handle, transform});

    auto helmet = assets->LoadModelAsync("DamagedHelmet.glb");
    m_Instances.push_back(SceneInstance{helmet->asset.handle, transform});

    // auto box = assets->LoadModelAsync("Box.glb");
    // m_Instances.push_back(SceneInstance{box->asset.handle, transform});

    //  auto free_stone_sphere = assets->LoadModelAsync("free_stone_sphere.glb");
    //   m_Instances.push_back(SceneInstance{free_stone_sphere->asset.handle, transform});

    //  auto AlphaBlendModeTest = assets->LoadModelAsync("Sponza/AlphaBlendModeTest.glb");
    //   m_Instances.push_back(SceneInstance{AlphaBlendModeTest->asset.handle, transform});


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

    m_DirectionalLight.SetDirection(pos);
    m_DirectionalLight.DebugDraw(renderer.get(), sunTextureIdx);


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
    renderer->UpdatePointLight(m_OmniLights.data(), m_OmniLights.size());

    // Directional Light Orthographic frustum
    glm::vec3 shadowCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    float halfSize = 5.0f;
    float shadowNear = 0.1f;
    float shadowFar = 30.0f;

    // glm::vec3 lightPos = m_DirectionalLight->GetVirtualPosition();
    glm::vec3 lightPos = -m_DirectionalLight.GetDirection() * 10.f;

    glm::mat4 lightView = glm::lookAt(lightPos, shadowCenter, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 lightProjection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, shadowNear, shadowFar);

    // inverse view gives camera axes in world space
    glm::mat4 invLightView = glm::inverse(lightView);

    glm::vec3 right = glm::normalize(glm::vec3(invLightView[0]));
    glm::vec3 up = glm::normalize(glm::vec3(invLightView[1]));

    // GLM/lookAt camera looks along local -Z
    glm::vec3 forward = -glm::normalize(glm::vec3(invLightView[2]));

    // Plane centers
    glm::vec3 nearCenter = lightPos + forward * shadowNear;
    glm::vec3 farCenter = lightPos + forward * shadowFar;

    // Near plane corners
    glm::vec3 LNB = nearCenter - right * halfSize - up * halfSize;  // Left Near Bottom
    glm::vec3 RNB = nearCenter + right * halfSize - up * halfSize;  // Right Near Bottom
    glm::vec3 LNT = nearCenter - right * halfSize + up * halfSize;  // Left Near Top
    glm::vec3 RNT = nearCenter + right * halfSize + up * halfSize;  // Right Near Top

    // Far plane corners
    glm::vec3 LFB = farCenter - right * halfSize - up * halfSize;  // Left Far Bottom
    glm::vec3 RFB = farCenter + right * halfSize - up * halfSize;  // Right Far Bottom
    glm::vec3 LFT = farCenter - right * halfSize + up * halfSize;  // Left Far Top
    glm::vec3 RFT = farCenter + right * halfSize + up * halfSize;  // Right Far Top

    // Near plane
    Fleur::Graphics::Color color = Fleur::Graphics::Color::Cyan();
    renderer->Debug().Line(LNB, RNB, color);
    renderer->Debug().Line(RNB, RNT, color);
    renderer->Debug().Line(RNT, LNT, color);
    renderer->Debug().Line(LNT, LNB, color);

    // Far plane
    renderer->Debug().Line(LFB, RFB, color);
    renderer->Debug().Line(RFB, RFT, color);
    renderer->Debug().Line(RFT, LFT, color);
    renderer->Debug().Line(LFT, LFB, color);

    // Side edges
    renderer->Debug().Line(LNB, LFB, color);
    renderer->Debug().Line(RNB, RFB, color);
    renderer->Debug().Line(LNT, LFT, color);
    renderer->Debug().Line(RNT, RFT, color);
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

Fleur::Graphics::RenderFrameData Scene::GetFrameData() const
{
    return Fleur::Graphics::RenderFrameData{{m_Camera->GetCameraForward(), m_Camera->GetView(), m_Camera->GetProjection()},
                                            {.dirIntens{glm::vec4(m_DirectionalLight.GetDirection(), m_DirectionalLight.GetIntensity())},
                                             .color = m_DirectionalLight.GetColor().ToVec4(),
                                             .pos = glm::vec4(m_DirectionalLight.GetVirtualPosition(), 1)}};
}

}  // namespace Fleur
