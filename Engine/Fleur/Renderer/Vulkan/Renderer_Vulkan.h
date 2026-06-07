#pragma once

#include <cassert>

#include "IRenderer.hpp"

using namespace Fleur::Graphics;

namespace vk
{

struct backend : public Fleur::Graphics::IRenderer
{
    // ---------- pImpl ----------
    struct impl;

    // Vrtual interface
    virtual ~backend() override;

    virtual void AddModel(const SVertexData* vertices, uint32_t verticesCount, const uint32_t* indecies, uint32_t indexCount, FLDrawItem* items,
                          uint32_t itemsCount) override;

    virtual void Update(SFLCameraData& cameraData) override;
    virtual void SubmitImageViews(SFLImageViewInfo* pInfo) override;

    virtual void CreateSkybox(AssetID id, SFLShaderStages shaderStages) override;
    virtual void SetSkybox(AssetID id) override;

    backend(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback);

    void StartResize() override;
    void EndResize(Fleur::SRect& rect) override;

    virtual void CreatePass(EFLPassKind kind, SFLShaderStages shaderStages) override;

    virtual void RegisterModel(AssetID model, const SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                               const FLDrawItem* primitives, uint32_t primitiveCount) override;
    virtual void UnregisterModel(AssetID model) override;
    virtual void RemoveTexture(AssetID texture) override;

    virtual void BeginFrame(SFLCameraData& cameraData) override;
    virtual void Draw(AssetID model, const glm::mat4& transform) override;
    virtual void EndFrame() override;

private:
    impl* pImpl;
};
}  // namespace vk