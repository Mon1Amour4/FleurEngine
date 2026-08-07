#pragma once

#include "../WindowPrimitives.hpp"
#include "Graphics.hpp"
#include "RenderViews.hpp"
#include "Shader.h"
#include "glm/glm.hpp"

#include <string_view>

namespace Fleur::Graphics
{

#pragma region Structs&Enums

enum class EFLPassKind
{
    Opaque,
    Shadow
};

enum class EFLShadowPassKind
{
    Directional,
    PointLight
};

struct SFLShaderBytecode
{
    const char* shaderCode{nullptr};
    uint32_t sizeBytes{0};
};
struct SFLShaderStages
{
    std::string_view vertex;
    std::string_view fragment;
    std::string_view geometry;
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
    virtual void SetShaderRegistry(const Fleur::Graphics::ShaderRegistry& shaders) = 0;
    virtual void SetShadowSceneBounds(const Fleur::Graphics::BoundingBox& bounds) = 0;

    virtual void CreateSkybox(AssetID id, SFLShaderStages shaderStages) = 0;
    virtual void SetSkybox(AssetID id) = 0;
    virtual void CreateFloor(AssetID texture, SFLShaderStages shaderStages, float height = 0.0f) = 0;
    virtual void SetFloor(AssetID texture, float height) = 0;

    virtual void CreatePass(EFLPassKind kind, SFLShaderStages shaderStages) = 0;
    virtual void CreateShadowPass(EFLShadowPassKind kind, SFLShaderStages shaderStages) = 0;
    virtual void ConfigureDebugDraw(const SFLDebugDrawShaders& shaders) = 0;
    virtual void ConfigureOverlay(SFLShaderStages shaderStages) = 0;

    // --- frame API (immediate, per-AssetID) ---
    virtual void RegisterModel(const SFLModelRegisterInfo& info) = 0;
    virtual void UnregisterModel(AssetID model) = 0;
    virtual void RemoveTexture(AssetID texture) = 0;

    virtual void BeginFrame(const Fleur::Graphics::RenderFrameData& frameData) = 0;
    virtual void Draw(AssetID model, const Fleur::Mat4& transform) = 0;
    virtual void EndFrame() = 0;

    // Debug geometry — primitives only. Composites (AABB/Sphere/...) decompose into
    // these on the frontend, so backends never duplicate the composite math.
    virtual void DrawLine(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 color, bool depthTest = true) = 0;
    virtual void DrawPoint(Fleur::Vec3 p, Fleur::Vec3 color, float size = 4.0f, bool depthTest = true) = 0;
    virtual void DrawQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, Fleur::Vec4 color, bool depthTest = true) = 0;
    virtual void DrawQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, uint32_t texture, bool depthTest = true) = 0;
    virtual void DrawBillboard(Fleur::Vec3 center, Fleur::Vec2 size, uint32_t texture, bool depthTest = true) = 0;
    virtual void DrawOverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, Fleur::Vec4 color) = 0;
    virtual void DrawOverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, uint32_t texture) = 0;
    virtual void DrawOverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec4 color) = 0;
    virtual void DrawOverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, uint32_t texture) = 0;
    virtual void DrawShadowMapOverlay(Fleur::Vec2 min, Fleur::Vec2 max) = 0;

    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) = 0;
};

}  // namespace Fleur::Graphics
