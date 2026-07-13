#pragma once

#include "../WindowPrimitives.hpp"
#include "Graphics.hpp"
#include "RenderViews.hpp"
#include "glm/glm.hpp"

namespace Fleur::Graphics
{

#pragma region Structs&Enums

enum class EFLPassKind
{
    Geometry,
    Shadow,
    Overlay
};

struct SFLShaderInfo
{
    const char* shaderCode{nullptr};
    uint32_t sizeBytes{0};
};
struct SFLShaderStages
{
    SFLShaderInfo vertex;
    SFLShaderInfo fragment;
};

struct SFLDebugDrawShaders
{
    SFLShaderStages primitives;
    SFLShaderStages geometry;
};

#pragma endregion

struct IRenderer
{
    virtual ~IRenderer() = default;

    virtual void UploadTextures(Fleur::Graphics::SFLImageViewInfo* pInfo) = 0;

    virtual void StartResize() = 0;
    virtual void EndResize(Fleur::SRect& rect) = 0;

    virtual void CreateSkybox(AssetID id, SFLShaderStages shaderStages) = 0;
    virtual void SetSkybox(AssetID id) = 0;

    virtual void CreatePass(EFLPassKind kind, SFLShaderStages shaderStages) = 0;
    virtual void ConfigureDebugDraw(const SFLDebugDrawShaders& shaders) = 0;
    virtual void ConfigureOverlay(SFLShaderStages shaderStages) = 0;

    // --- frame API (immediate, per-AssetID) ---
    virtual void RegisterModel(const SFLModelRegisterInfo& info) = 0;
    virtual void UnregisterModel(AssetID model) = 0;
    virtual void RemoveTexture(AssetID texture) = 0;

    virtual void BeginFrame(const Fleur::Graphics::RenderFrameData& frameData) = 0;
    virtual void Draw(AssetID model, const glm::mat4& transform) = 0;
    virtual void EndFrame() = 0;

    // Debug geometry — primitives only. Composites (AABB/Sphere/...) decompose into
    // these on the frontend, so backends never duplicate the composite math.
    virtual void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color, bool depthTest = true) = 0;
    virtual void DrawPoint(glm::vec3 p, glm::vec3 color, float size = 4.0f, bool depthTest = true) = 0;
    virtual void DrawQuad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec4 color, bool depthTest = true) = 0;
    virtual void DrawQuad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, uint32_t texture, bool depthTest = true) = 0;
    virtual void DrawBillboard(glm::vec3 center, glm::vec2 size, uint32_t texture, bool depthTest = true) = 0;
    virtual void DrawOverlayQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, glm::vec4 color) = 0;
    virtual void DrawOverlayQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, uint32_t texture) = 0;
    virtual void DrawOverlayTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 color) = 0;
    virtual void DrawOverlayTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, uint32_t texture) = 0;
    virtual void DrawShadowMapOverlay(glm::vec2 min, glm::vec2 max) = 0;

    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) = 0;
};

}  // namespace Fleur::Graphics
