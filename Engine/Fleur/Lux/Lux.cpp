#include "Lux.h"

#include "Application.h"
#include "AssetsManager.h"
#include "Image2D.h"
#include "Log.h"
#include "Lux.h"
#include "Services/ServiceLocator.h"
#include "Shader.h"

#include <array>
#include <cmath>
#include <utility>

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

void Renderer::OnInit()
{
}  // the backend is created on demand via SetBackend (runtime-selected)

void Renderer::Initialize(Fleur::Graphics::EGraphicsAPI api)
{
    if (m_Backend)
        return;

    m_Api = api;
    initBackend();
}

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
    FL_CORE_ASSERT(m_ShaderRegistry, "Renderer shader registry was not initialized");

    auto fallbackAsset = assetsManager->Get<Fleur::Graphics::Image2D>("wall_placeholder2.png");
    Fleur::Graphics::SFLImageView fallbackView = fallbackAsset.obj->GetView();
    fallbackView.ID = fallbackAsset.handle.id;

    Fleur::SRect framebufferSize = application.GetWindow().GetFramebufferSize();
    void* window = application.GetWindow().GetNativeHandle();
    bool validation = true;

    if (m_Api == Fleur::Graphics::EGraphicsAPI::OpenGL)
        m_Backend = new gl::backend(validation, window, framebufferSize, fallbackView, m_MaxPointLights, m_CascadeCount,
                                     m_DirectionalLightSampling, m_PointLightSampling, Fleur::Log::GetCoreLogger());
    else
        m_Backend = new vk::backend(validation, window, framebufferSize, fallbackView, m_MaxPointLights, m_CascadeCount,
                                     m_DirectionalLightSampling, m_PointLightSampling, Fleur::Log::GetCoreLogger());

    m_Backend->SetShaderRegistry(*m_ShaderRegistry);
    // TEMP_DEBUG_F4_NORMAL_MAP: keep backend state aligned with the frontend default.
    m_Backend->SetNormalMappingEnabled(m_NormalMappingEnabled);

    m_Debug = new DebugDraw(m_Backend);
    InitializePipelines();
}

void Renderer::InitializePipelines()
{
    FL_CORE_ASSERT(m_Backend, "Renderer backend was not initialized");

    auto assetsManager = Fleur::ServiceLocator::instance().GetService<Fleur::AssetsManager>();
    auto fallbackAsset = assetsManager->Get<Fleur::Graphics::Image2D>("wall_placeholder2.png");

    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::Opaque, {"opaqueVertex", "opaqueFragment"});
    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::Deferred, {"opaqueVertex", "deferredFragment"});
    m_Backend->CreatePass(Fleur::Graphics::EFLPassKind::DeferredLighting, {"deferredLightingVertex", "deferredLightingFragment"});
    m_Backend->ConfigureDebugDraw({.primitives = {"debugVertex", "debugFragment"},
                                   .geometry = {"debugGeometryVertex", "debugGeometryFragment"}});
    m_Backend->ConfigureOverlay({"overlayVertex", "overlayFragment"});

    m_Backend->CreateShadowPass(Fleur::Graphics::EFLShadowPassKind::Directional,
                                {.vertex = "shadowVertex", .fragment = "shadowFragment", .geometry = "shadowGeometry"});

    m_Backend->CreateShadowPass(Fleur::Graphics::EFLShadowPassKind::PointLight,
                                 {.vertex = "pointLightShadowVertex", .fragment = "pointLightShadowFragment", .geometry = "pointLightShadowGeometry"});

    m_Backend->CreateSkybox(fallbackAsset.handle.id, {"skyboxVertex", "skyboxFragment"});
}

void Renderer::OnShutdown()
{
    delete m_Debug;
    m_Debug = nullptr;

    delete m_Backend;
    m_Backend = nullptr;
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
    if (frameData.pointLightsDirty)
        m_Backend->UpdatePointLight(frameData.pointLights.data(), static_cast<uint32_t>(frameData.pointLights.size()));

    m_Backend->BeginFrame(frameData);
}
void Renderer::Draw(AssetID model, const Fleur::Mat4& transform)
{
    m_Backend->Draw(model, transform);
}

void Renderer::RegisterShadowInstance(AssetID model, const Fleur::Mat4& transform,
                                      const Fleur::Graphics::BoundingBox& localBounds)
{
    (void)model;
    const Fleur::Vec3 min = localBounds.GetMin();
    const Fleur::Vec3 max = localBounds.GetMax();
    const auto corner = [&](float x, float y, float z) { return Fleur::Vec3(transform * Fleur::Vec4(x, y, z, 1.0f)); };
    const Fleur::Vec3 corners[8] = {
        corner(min.x, min.y, min.z), corner(max.x, min.y, min.z), corner(max.x, max.y, min.z), corner(min.x, max.y, min.z),
        corner(min.x, min.y, max.z), corner(max.x, min.y, max.z), corner(max.x, max.y, max.z), corner(min.x, max.y, max.z)};

    for (const auto& worldCorner : corners)
        m_ShadowSceneBounds.UpdateBoundingBox(worldCorner, worldCorner);

    m_HasShadowSceneBounds = true;
    m_Backend->SetShadowSceneBounds(m_ShadowSceneBounds);
}
void Renderer::CreateFloor(Fleur::Graphics::AssetID texture, float height)
{
    if (!m_Backend)
        return;

    m_Backend->CreateFloor(texture, {"floorVertex", "floorFragment"}, height);
}

void Renderer::SetFloor(Fleur::Graphics::AssetID texture, float height)
{
    if (m_Backend)
        m_Backend->SetFloor(texture, height);
}
void Renderer::EndFrame()
{
    if (m_ShowShadowMapPreview)
        ShadowMapPreview(Fleur::Vec2(0.62f, 0.62f), Fleur::Vec2(0.96f, 0.96f));

    m_Backend->EndFrame();
}

void Renderer::OverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, Fleur::Graphics::Color color)
{
    if (!m_Backend)
        return;

    m_Backend->DrawOverlayQuad(a, b, c, d, color.ToVec4());
}

void Renderer::OverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, Fleur::Graphics::AssetID texture)
{
    if (!m_Backend)
        return;

    m_Backend->DrawOverlayQuad(a, b, c, d, texture);
}

void Renderer::OverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Graphics::Color color)
{
    if (!m_Backend)
        return;

    m_Backend->DrawOverlayTriangle(a, b, c, color.ToVec4());
}

void Renderer::OverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Graphics::AssetID texture)
{
    if (!m_Backend)
        return;

    m_Backend->DrawOverlayTriangle(a, b, c, texture);
}

void Renderer::ShadowMapPreview(Fleur::Vec2 min, Fleur::Vec2 max)
{
    if (!m_Backend)
        return;

    m_Backend->DrawShadowMapOverlay(min, max, m_ShadowMapPreviewLayer);
}

void Renderer::ToggleShadowMapPreview()
{
    constexpr int32_t previewLayerCount = 5;
    ++m_ShadowMapPreviewLayer;
    if (m_ShadowMapPreviewLayer >= previewLayerCount)
        m_ShadowMapPreviewLayer = -1;
    m_ShowShadowMapPreview = m_ShadowMapPreviewLayer >= 0;
}

// TEMP_DEBUG_F4_NORMAL_MAP: remove after normal-map debugging.
void Renderer::ToggleNormalMapping()
{
    m_NormalMappingEnabled = !m_NormalMappingEnabled;
    if (m_Backend)
        m_Backend->SetNormalMappingEnabled(m_NormalMappingEnabled);
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
void DebugDraw::Line(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Graphics::Color color, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawLine(a, b, color.ToVec3(), depthTest);
}

void DebugDraw::Point(Fleur::Vec3 p, Color color, float size, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawPoint(p, color.ToVec3(), size, depthTest);
}

void DebugDraw::Quad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, Fleur::Graphics::Color color, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawQuad(a, b, c, d, color.ToVec4(), depthTest);
}

void DebugDraw::Quad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, AssetID texture, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawQuad(a, b, c, d, texture, depthTest);
}

void DebugDraw::Billboard(Fleur::Vec3 center, Fleur::Vec2 size, AssetID texture, bool depthTest)
{
    if (!m_Backend)
        return;

    m_Backend->DrawBillboard(center, size, texture, depthTest);
}

void DebugDraw::DrawAxes()
{
    Fleur::Vec3 origin{0, 0, 0};
    float length{1000};

    const Fleur::Vec3 xAxis(length, 0.0f, 0.0f);
    const Fleur::Vec3 yAxis(0.0f, length, 0.0f);
    const Fleur::Vec3 zAxis(0.0f, 0.0f, length);

    Line(origin - xAxis, origin + xAxis, Fleur::Graphics::Color::Red());
    Line(origin - yAxis, origin + yAxis, Fleur::Graphics::Color::Green());
    Line(origin - zAxis, origin + zAxis, Fleur::Graphics::Color::Blue());
}

void DebugDraw::Sphere(Fleur::Vec3 center, float radius, Color color, int segments)
{
    if (!m_Backend || radius <= 0.0f || segments < 3)
        return;

    constexpr float twoPi = 6.28318530718f;
    const Fleur::Vec3 xAxis(1.0f, 0.0f, 0.0f);
    const Fleur::Vec3 yAxis(0.0f, 1.0f, 0.0f);
    const Fleur::Vec3 zAxis(0.0f, 0.0f, 1.0f);

    auto drawCircle = [&](Fleur::Vec3 axisA, Fleur::Vec3 axisB)
    {
        Fleur::Vec3 previous = center + axisA * radius;
        for (int i = 1; i <= segments; ++i)
        {
            const float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);
            const Fleur::Vec3 current = center + axisA * (std::cos(angle) * radius) + axisB * (std::sin(angle) * radius);
            Line(previous, current, color);
            previous = current;
        }
    };

    drawCircle(xAxis, yAxis);
    drawCircle(xAxis, zAxis);
    drawCircle(yAxis, zAxis);
}

void DebugDraw::Frustum(const Fleur::Mat4& invViewProj, Color color)
{
    const std::array<Fleur::Vec4, 8> clipCorners = {
        Fleur::Vec4(-1.f, -1.f, 0.f, 1.f), Fleur::Vec4(1.f, -1.f, 0.f, 1.f), Fleur::Vec4(1.f, 1.f, 0.f, 1.f), Fleur::Vec4(-1.f, 1.f, 0.f, 1.f),
        Fleur::Vec4(-1.f, -1.f, 1.f, 1.f), Fleur::Vec4(1.f, -1.f, 1.f, 1.f), Fleur::Vec4(1.f, 1.f, 1.f, 1.f), Fleur::Vec4(-1.f, 1.f, 1.f, 1.f),
    };

    std::array<Fleur::Vec3, 8> worldCorners{};
    for (size_t i = 0; i < clipCorners.size(); ++i)
    {
        const Fleur::Vec4 world = invViewProj * clipCorners[i];
        worldCorners[i] = Fleur::Vec3(world) / world.w;
    }

    static constexpr std::array<std::pair<uint32_t, uint32_t>, 12> edges = {
        std::make_pair(0u, 1u), std::make_pair(1u, 2u), std::make_pair(2u, 3u), std::make_pair(3u, 0u), std::make_pair(4u, 5u),
        std::make_pair(5u, 6u), std::make_pair(6u, 7u), std::make_pair(7u, 4u), std::make_pair(0u, 4u), std::make_pair(1u, 5u),
        std::make_pair(2u, 6u), std::make_pair(3u, 7u),
    };

    for (const auto& [a, b] : edges)
        Line(worldCorners[a], worldCorners[b], color);
}

void DebugDraw::BoundingBox(Fleur::Graphics::BoundingBox boundingBox, Fleur::Mat4 transform, Color color, bool depthTest)
{
    if (!m_Backend)
        return;

    // Decompose to 12 edges here (once), forward as lines — backends stay composite-free.
    auto corner = [&](float x, float y, float z) { return Fleur::Vec3(transform * Fleur::Vec4(x, y, z, 1.0f)); };
    const Fleur::Vec3& mn = boundingBox.GetMin();
    const Fleur::Vec3& mx = boundingBox.GetMax();
    Fleur::Vec3 c[8] = {corner(mn.x, mn.y, mn.z), corner(mx.x, mn.y, mn.z), corner(mx.x, mx.y, mn.z), corner(mn.x, mx.y, mn.z),
                      corner(mn.x, mn.y, mx.z), corner(mx.x, mn.y, mx.z), corner(mx.x, mx.y, mx.z), corner(mn.x, mx.y, mx.z)};

    static const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    Fleur::Vec3 col = color.ToVec3();
    for (const auto& e : edges) m_Backend->DrawLine(c[e[0]], c[e[1]], col, depthTest);
}

}  // namespace Lux
