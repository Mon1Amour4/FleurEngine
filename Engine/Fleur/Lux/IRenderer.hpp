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
    Opaque,
    Transparent,
    AABB_DEBUG,
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

    // --- frame API (immediate, per-AssetID) ---
    virtual void RegisterModel(const SFLModelRegisterInfo& info) = 0;
    virtual void UnregisterModel(AssetID model) = 0;
    virtual void RemoveTexture(AssetID texture) = 0;

    virtual void BeginFrame(Fleur::Graphics::SFLCameraData& cameraData) = 0;
    virtual void Draw(AssetID model, const glm::mat4& transform) = 0;
    virtual void EndFrame() = 0;

    // Debug geometry — primitives only. Composites (AABB/Sphere/...) decompose into
    // these on the frontend, so backends never duplicate the composite math.
    virtual void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color, bool depthTest = true) = 0;
    virtual void DrawPoint(glm::vec3 p, glm::vec3 color, float size = 4.0f, bool depthTest = true) = 0;

    virtual void SetDirectionalLight(glm::vec3 direction, glm::vec4 color, float intensity) = 0;
    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) = 0;
};

}  // namespace Fleur::Graphics
