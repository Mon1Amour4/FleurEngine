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
    void Line(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Graphics::Color color, bool depthTest = true);
    void Point(Fleur::Math::vec3 p, Fleur::Graphics::Color color, float size = 4.0f, bool depthTest = true);
    void Quad(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 c, Fleur::Math::vec3 d, Fleur::Graphics::Color color, bool depthTest = true);
    void Quad(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 c, Fleur::Math::vec3 d, AssetID texture, bool depthTest = true);
    void Billboard(Fleur::Math::vec3 center, Fleur::Math::vec2 size, AssetID texture, bool depthTest = true);

    void DrawAxes();

    // --- Composites (constructs from Line\Point) ---
    void BoundingBox(Fleur::Graphics::BoundingBox boundingBox, Fleur::Math::mat4 transform, Color color, bool depthTest = true);  // oriented
    void Sphere(Fleur::Math::vec3 center, float radius, Color color, int segments = 16);
    void Ray(Fleur::Math::vec3 origin, Fleur::Math::vec3 dir, float length, Color color);
    void Axes(const Fleur::Math::mat4& transform, float size = 1.0f);  // RGB = XYZ ???
    void Frustum(const Fleur::Math::mat4& invViewProj, Color color);

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
    void Draw(Fleur::Graphics::AssetID model, const Fleur::Math::mat4& transform);
    void EndFrame();
    void OverlayQuad(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec2 d, Fleur::Graphics::Color color);
    void OverlayQuad(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec2 d, Fleur::Graphics::AssetID texture);
    void OverlayTriangle(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Graphics::Color color);
    void OverlayTriangle(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Graphics::AssetID texture);
    void ShadowMapPreview(Fleur::Math::vec2 min, Fleur::Math::vec2 max);
    void ToggleShadowMapPreview();
    bool IsShadowMapPreviewEnabled() const
    {
        return m_ShowShadowMapPreview;
    }

    // Window / engine.
    void StartResize();
    void EndResize(Fleur::SRect& rect);
    void SetSkybox(Fleur::Graphics::AssetID id);
    void SetVSync(bool active)
    {
        m_Vsync = active;
    }
    bool IsVSync() const
    {
        return m_Vsync;
    }

    // Runtime backend selection / live switching (all backends are compiled in).
    void SetBackend(Fleur::Graphics::EGraphicsAPI api);
    Fleur::Graphics::EGraphicsAPI GetBackendApi() const
    {
        return m_Api;
    }

    // Debug
    DebugDraw& Debug()
    {
        return *m_Debug;
    }

    void UpdatePointLight(const Fleur::Graphics::OmniLight* pLight, uint32_t lightCount);

protected:
    void OnInit();
    void OnShutdown();

private:
    DebugDraw* m_Debug{nullptr};

    Fleur::Graphics::SFLShaderInfo shaderInfo(Fleur::Graphics::Shader* shader);
    void initBackend();

    Fleur::Graphics::IRenderer* m_Backend{nullptr};
    Fleur::Graphics::EGraphicsAPI m_Api{Fleur::Graphics::EGraphicsAPI::Vulkan};
    bool m_Vsync{true};
    bool m_ShowShadowMapPreview{false};
};
}  // namespace Lux
