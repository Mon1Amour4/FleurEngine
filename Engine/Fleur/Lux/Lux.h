#pragma once

#include <Fleur/Math/Math.hpp>

#include "Color.h"
#include "IRenderer.hpp"  // IRenderer + DTOs
#include "OmniLight.h"
#include "Services/ServiceInterfaces.hpp"  // Service<>

namespace Fleur::Graphics
{
class Shader;
}

namespace Lux
{
using AssetID = Fleur::Graphics::AssetID;
using Color = Fleur::Graphics::Color;
using IRenderer = Fleur::Graphics::IRenderer;

class DebugDraw
{
public:
    DebugDraw(IRenderer* renderer)
        : m_Backend(renderer) {};

    // --- Primitives ---
    void Line(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Graphics::Color color, bool depthTest = true);
    void Point(Fleur::Vec3 p, Fleur::Graphics::Color color, float size = 4.0f, bool depthTest = true);
    void Quad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, Fleur::Graphics::Color color, bool depthTest = true);
    void Quad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, AssetID texture, bool depthTest = true);
    void Billboard(Fleur::Vec3 center, Fleur::Vec2 size, AssetID texture, bool depthTest = true);

    void DrawAxes();

    // --- Composites (constructs from Line\Point) ---
    void BoundingBox(Fleur::Graphics::BoundingBox boundingBox, Fleur::Mat4 transform, Color color, bool depthTest = true);  // oriented
    void Sphere(Fleur::Vec3 center, float radius, Color color, int segments = 16);
    void Ray(Fleur::Vec3 origin, Fleur::Vec3 dir, float length, Color color);
    void Axes(const Fleur::Mat4& transform, float size = 1.0f);  // RGB = XYZ ???
    void Frustum(const Fleur::Mat4& invViewProj, Color color);

private:
    IRenderer* m_Backend{nullptr};
};
// Frontend renderer service. Owns the graphics backend (IRenderer), drives the frame.
//   SetBackend: (re)creates the backend + passes + skybox (runtime-selected; OnInit is a no-op).
//   Application: drives the frame loop (BeginFrame/Draw via Scene/EndFrame) + Register/Upload.
class Renderer : public Fleur::Service<Renderer>
{
public:
    friend struct Fleur::Service<Renderer>;

    Renderer();
    ~Renderer();

    // Resource lifetime (retained). Forward on asset load, inverse on evict.
    void Register(const Fleur::Graphics::SFLModelRegisterInfo& info);

    void Unregister(Fleur::Graphics::AssetID model);
    void UploadTextures(Fleur::Graphics::SFLImageViewInfo* info);
    void RemoveTexture(Fleur::Graphics::AssetID texture);

    // Frame (immediate-mode).
    void BeginFrame(const Fleur::Graphics::RenderFrameData& frameData);
    void Draw(Fleur::Graphics::AssetID model, const Fleur::Mat4& transform);
    void RegisterShadowInstance(Fleur::Graphics::AssetID model, const Fleur::Mat4& transform,
                                const Fleur::Graphics::BoundingBox& localBounds);
    void SetFloor(Fleur::Graphics::AssetID texture, float height);
    void EndFrame();
    void OverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, Fleur::Graphics::Color color);
    void OverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, Fleur::Graphics::AssetID texture);
    void OverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Graphics::Color color);
    void OverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Graphics::AssetID texture);
    void ShadowMapPreview(Fleur::Vec2 min, Fleur::Vec2 max);
    void ToggleShadowMapPreview();
    bool IsShadowMapPreviewEnabled() const
    {
        return m_ShowShadowMapPreview;
    }

    // Window / engine.
    void StartResize();
    void EndResize(Fleur::SRect& rect);
    void SetSkybox(Fleur::Graphics::AssetID id);
    void CreateFloor(Fleur::Graphics::AssetID texture, float height = 0.0f);
    void SetVSync(bool active)
    {
        m_Vsync = active;
    }
    bool IsVSync() const
    {
        return m_Vsync;
    }

    // Runtime backend selection / live switching (all backends are compiled in).
    void Initialize(Fleur::Graphics::EGraphicsAPI api);
    void SetBackend(Fleur::Graphics::EGraphicsAPI api);
    void SetShaderRegistry(const Fleur::Graphics::ShaderRegistry& shaders)
    {
        m_ShaderRegistry = &shaders;
    }
    void SetMaxPointLights(uint32_t maxPointLights)
    {
        m_MaxPointLights = maxPointLights;
    }
    uint32_t GetMaxPointLights() const
    {
        return m_MaxPointLights;
    }
    Fleur::Graphics::EGraphicsAPI GetBackendApi() const
    {
        return m_Api;
    }

    // Debug
    DebugDraw& Debug()
    {
        return *m_Debug;
    }

protected:
    void OnInit();
    void OnShutdown();

private:
    DebugDraw* m_Debug{nullptr};

    void initBackend();
    void InitializePipelines();

    Fleur::Graphics::IRenderer* m_Backend{nullptr};
    Fleur::Graphics::EGraphicsAPI m_Api{Fleur::Graphics::EGraphicsAPI::Vulkan};
    bool m_Vsync{true};
    bool m_ShowShadowMapPreview{false};
    uint32_t m_MaxPointLights{128};
    const Fleur::Graphics::ShaderRegistry* m_ShaderRegistry{nullptr};
    Fleur::Graphics::BoundingBox m_ShadowSceneBounds{};
    bool m_HasShadowSceneBounds{false};
};
}  // namespace Lux
