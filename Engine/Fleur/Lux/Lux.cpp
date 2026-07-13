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

    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::Geometry, {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("opaqueVertex").obj),
                                                                   shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("opaqueFragment").obj)});

    m_Backend->ConfigureDebugDraw({.primitives = {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("debugVertex").obj),
                                                  shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("debugFragment").obj)},
                                   .geometry = {shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("debugGeometryVertex").obj),
                                                shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("debugGeometryFragment").obj)}});
    m_Backend->ConfigureOverlay({shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("overlayVertex").obj),
                                 shaderInfo(assetsManager->Get<Fleur::Graphics::Shader>("overlayFragment").obj)});

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

void Renderer::BeginFrame(const Fleur::Graphics::RenderFrameData& frameData)
{
    m_Backend->BeginFrame(frameData);
}
void Renderer::Draw(AssetID model, const glm::mat4& transform)
{
    m_Backend->Draw(model, transform);
}
void Renderer::EndFrame()
{
    if (m_ShowShadowMapPreview)
        ShadowMapPreview(glm::vec2(0.62f, 0.62f), glm::vec2(0.96f, 0.96f));

    m_Backend->EndFrame();
}

void Renderer::OverlayQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, Fleur::Graphics::Color color)
{
    if (!m_Backend)
        return;

    m_Backend->DrawOverlayQuad(a, b, c, d, color.ToVec4());
}

void Renderer::OverlayQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, Fleur::Graphics::AssetID texture)
{
    if (!m_Backend)
        return;

    m_Backend->DrawOverlayQuad(a, b, c, d, texture);
}

void Renderer::OverlayTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, Fleur::Graphics::Color color)
{
    if (!m_Backend)
        return;

    m_Backend->DrawOverlayTriangle(a, b, c, color.ToVec4());
}

void Renderer::OverlayTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, Fleur::Graphics::AssetID texture)
{
    if (!m_Backend)
        return;

    m_Backend->DrawOverlayTriangle(a, b, c, texture);
}

void Renderer::ShadowMapPreview(glm::vec2 min, glm::vec2 max)
{
    if (!m_Backend)
        return;

    m_Backend->DrawShadowMapOverlay(min, max);
}

void Renderer::ToggleShadowMapPreview()
{
    m_ShowShadowMapPreview = !m_ShowShadowMapPreview;
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

void DebugDraw::Quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, Fleur::Graphics::Color color, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawQuad(a, b, c, d, color.ToVec4(), depthTest);
}

void DebugDraw::Quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, AssetID texture, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawQuad(a, b, c, d, texture, depthTest);
}

void DebugDraw::Billboard(glm::vec3 center, glm::vec2 size, AssetID texture, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawBillboard(center, size, texture, depthTest);
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
