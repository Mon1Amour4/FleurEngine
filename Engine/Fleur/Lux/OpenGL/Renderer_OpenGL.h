#pragma once

#include <cassert>

#include "IRenderer.hpp"

using namespace Fleur::Graphics;

namespace gl
{
// OpenGL backend. Mirrors vk::backend: a thin public shim over a private impl,
// implementing the shared IRenderer contract (models / textures / skybox).
struct backend : public Fleur::Graphics::IRenderer
{
    struct impl;

    backend(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback);
    virtual ~backend() override;

    void CreatePass(EFLPassKind kind, SFLShaderStages shaderStages) override;
    void CreateSkybox(AssetID id, SFLShaderStages shaderStages) override;
    void SetSkybox(AssetID id) override;

    void RegisterModel(AssetID model, const SVertexData* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount,
                       const FLDrawItem* primitives, uint32_t primitiveCount) override;
    void UnregisterModel(AssetID model) override;
    void UploadTextures(SFLImageViewInfo* pInfo) override;
    void RemoveTexture(AssetID texture) override;

    void BeginFrame(SFLCameraData& cameraData) override;
    void Draw(AssetID model, const glm::mat4& transform) override;
    void EndFrame() override;

    void StartResize() override;
    void EndResize(Fleur::SRect& rect) override;

private:
    impl* pImpl;
};
}  // namespace gl
