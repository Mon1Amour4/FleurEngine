#pragma once

#include <glm/glm.hpp>

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
    void Line(glm::vec3 a, glm::vec3 b, Fleur::Graphics::Color color, bool depthTest = true);
    void Point(glm::vec3 p, Fleur::Graphics::Color color, float size = 4.0f, bool depthTest = true);
    void Quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, Fleur::Graphics::Color color, bool depthTest = true);
    void Quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, AssetID texture, bool depthTest = true);

    void DrawAxes();

    // --- Composites (constructs from Line\Point) ---
    void BoundingBox(Fleur::Graphics::BoundingBox boundingBox, glm::mat4 transform, Color color, bool depthTest = true);  // oriented
    void Sphere(glm::vec3 center, float radius, Color color, int segments = 16);
    void Ray(glm::vec3 origin, glm::vec3 dir, float length, Color color);
    void Axes(const glm::mat4& transform, float size = 1.0f);  // RGB = XYZ ???
    void Frustum(const glm::mat4& invViewProj, Color color);

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
    void Draw(Fleur::Graphics::AssetID model, const glm::mat4& transform);
    void EndFrame();

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
};
}  // namespace Lux
