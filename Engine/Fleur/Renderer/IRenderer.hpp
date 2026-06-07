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

    virtual void AddModel(const SVertexData* vertices, uint32_t verticesCount, const uint32_t* indecies, uint32_t indexCount, FLDrawItem* items,
                          uint32_t itemsCount) = 0;

    virtual void Update(Fleur::Graphics::SFLCameraData& cameraData) = 0;
    virtual void SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo) = 0;

    virtual void StartResize() = 0;
    virtual void EndResize(Fleur::SRect& rect) = 0;

    virtual void CreateSkybox(AssetID id, SFLShaderStages shaderStages) = 0;
    virtual void SetSkybox(AssetID id) = 0;

    virtual void CreatePass(EFLPassKind kind, SFLShaderStages shaderStages) = 0;

    // --- new frame API (cutover in progress; AddModel/Update retire once Lux + Application switch over) ---
    virtual void RegisterModel(AssetID model, const SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                               const FLDrawItem* primitives, uint32_t primitiveCount) = 0;
    virtual void UnregisterModel(AssetID model) = 0;
    virtual void RemoveTexture(AssetID texture) = 0;

    virtual void BeginFrame(Fleur::Graphics::SFLCameraData& cameraData) = 0;
    virtual void Draw(AssetID model, const glm::mat4& transform) = 0;
    virtual void EndFrame() = 0;
};

}  // namespace Fleur::Graphics
