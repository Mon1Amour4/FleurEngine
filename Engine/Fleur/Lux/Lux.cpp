#include "Lux.h"

#include "Application.h"
#include "AssetsManager.h"
#include "Image2D.h"
#include "Lux.h"
#include "Services/ServiceLocator.h"
#include "Shader.h"

// All backends are compiled in; the active one is selected at runtime.
#include "OpenGL/Renderer_OpenGL.h"
#include "Vulkan/Renderer_Vulkan.h"

namespace Lux
{

Renderer::Renderer()
{
}
Renderer::~Renderer()
{
    OnShutdown();
}

void Renderer::SetDirectionalLight(glm::vec3 direction, Fleur::Graphics::Color color, float intensity)
{
    m_Backend->SetDirectionalLight(direction, color.ToVec4(), glm::min(1.0f, intensity));
}

void Renderer::UpdatePointLight(const Fleur::Graphics::OmniLight* pLight, uint32_t lightCount)
{
    if (lightCount == 0)
        return;

    std::vector<SFLPointLight> lights;
    lights.reserve(lightCount);
    for (size_t i = 0; i < lightCount; i++)
    {
        auto& light = lights.emplace_back();
        light.pos = pLight[i].GetPosition();
        light.radius = pLight[i].GetRadius();
        light.color = pLight[i].GetColor().ToVec3();
        light.intensity = pLight[i].GetIntensity();
    }
    m_Backend->UpdatePointLight(lights.data(), lightCount);
}

void Renderer::OnInit()
{
}  // the backend is created on demand via SetBackend (runtime-selected)

void Renderer::SetBackend(Fleur::Graphics::EGraphicsAPI api)
{
    if (m_Backend && m_Api == api)
        return;

    delete m_Backend;
    m_Backend = nullptr;
    m_Api = api;
    initBackend();
}

void Renderer::initBackend()
{
    Fleur::Application& application = Fleur::Application::instance();
    auto assetsManager = Fleur::ServiceLocator::instance().GetService<Fleur::AssetsManager>();

    auto fallbackAsset = assetsManager->Get<Fleur::Graphics::Image2D>("wall_placeholder2.png");
    Fleur::Graphics::SFLImageView fallbackView = fallbackAsset.obj->GetView();
    fallbackView.ID = fallbackAsset.handle.id;

    Fleur::SRect framebufferSize = application.GetWindow().GetFramebufferSize();
    void* window = application.GetWindow().GetNativeHandle();
    bool validation = true;

    if (m_Api == Fleur::Graphics::EGraphicsAPI::OpenGL)
        m_Backend = new gl::backend(validation, window, framebufferSize, fallbackView);
    else
        m_Backend = new vk::backend(validation, window, framebufferSize, fallbackView);

    m_Debug = new DebugDraw(m_Backend);

    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::Opaque, {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("opaqueVertex").obj),
                                                                 shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("opaqueFragment").obj)});

    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::Transparent, {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("opaqueVertex").obj),
                                                                      shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("opaqueFragment").obj)});

    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::AABB_DEBUG, {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("debugVertex").obj),
                                                                     shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("debugFragment").obj)});

    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::Shadow, {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("shadowVertex").obj),
                                                                 shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("shadowFragment").obj)});

    m_Backend->CreateSkybox(fallbackAsset.handle.id, {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("skyboxVertex").obj),
                                                      shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("skyboxFragment").obj)});
}

void Renderer::OnShutdown()
{
    delete m_Debug;
    m_Debug = nullptr;

    delete m_Backend;
    m_Backend = nullptr;
}

Fleur::Graphics::SFLShaderInfo Renderer::shaderInfo(Fleur::Graphics::Shader* shader)
{
    if (!shader)
        return Fleur::Graphics::SFLShaderInfo();

    return {shader->GetShaderCode(), shader->GetShaderCodeSizeB()};
}

void Renderer::Register(const Fleur::Graphics::SFLModelRegisterInfo& info)
{
    m_Backend->RegisterModel(info);
}
void Renderer::Unregister(AssetID model)
{
    m_Backend->UnregisterModel(model);
}
void Renderer::UploadTextures(Fleur::Graphics::SFLImageViewInfo* info)
{
    m_Backend->UploadTextures(info);
}
void Renderer::RemoveTexture(AssetID texture)
{
    m_Backend->RemoveTexture(texture);
}

void Renderer::BeginFrame(const CameraView& camera)
{
    Fleur::Graphics::SFLCameraData data{camera.dir, camera.view, camera.proj};
    m_Backend->BeginFrame(data);
}
void Renderer::Draw(AssetID model, const glm::mat4& transform)
{
    m_Backend->Draw(model, transform);
}
void Renderer::EndFrame()
{
    m_Backend->EndFrame();
}

void Renderer::StartResize()
{
    m_Backend->StartResize();
}
void Renderer::EndResize(Fleur::SRect& rect)
{
    m_Backend->EndResize(rect);
}
void Renderer::SetSkybox(AssetID id)
{
    m_Backend->SetSkybox(id);
}

// Debug
void DebugDraw::Line(glm::vec3 a, glm::vec3 b, Fleur::Graphics::Color color, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawLine(a, b, color.ToVec3(), depthTest);
}

void DebugDraw::Point(glm::vec3 p, Color color, float size, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawPoint(p, color.ToVec3(), size, depthTest);
}

void DebugDraw::DrawAxes()
{
    glm::vec3 origin{0, 0, 0};
    float length{1000};

    const glm::vec3 xAxis(length, 0.0f, 0.0f);
    const glm::vec3 yAxis(0.0f, length, 0.0f);
    const glm::vec3 zAxis(0.0f, 0.0f, length);

    Line(origin - xAxis, origin + xAxis, Fleur::Graphics::Color::Red());
    Line(origin - yAxis, origin + yAxis, Fleur::Graphics::Color::Green());
    Line(origin - zAxis, origin + zAxis, Fleur::Graphics::Color::Blue());
}

void DebugDraw::BoundingBox(Fleur::Graphics::BoundingBox boundingBox, glm::mat4 transform, Color color, bool depthTest)
{
    if (!m_Backend)
        return;

    // Decompose to 12 edges here (once), forward as lines — backends stay composite-free.
    auto corner = [&](float x, float y, float z) { return glm::vec3(transform * glm::vec4(x, y, z, 1.0f)); };
    const glm::vec3& mn = boundingBox.GetMin();
    const glm::vec3& mx = boundingBox.GetMax();
    glm::vec3 c[8] = {corner(mn.x, mn.y, mn.z), corner(mx.x, mn.y, mn.z), corner(mx.x, mx.y, mn.z), corner(mn.x, mx.y, mn.z),
                      corner(mn.x, mn.y, mx.z), corner(mx.x, mn.y, mx.z), corner(mx.x, mx.y, mx.z), corner(mn.x, mx.y, mx.z)};

    static const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    glm::vec3 col = color.ToVec3();
    for (const auto& e : edges) m_Backend->DrawLine(c[e[0]], c[e[1]], col, depthTest);
}

}  // namespace Lux
