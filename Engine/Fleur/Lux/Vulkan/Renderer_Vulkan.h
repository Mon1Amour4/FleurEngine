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

    virtual void UploadTextures(SFLImageViewInfo* pInfo) override;

    virtual void CreateSkybox(AssetID id, SFLShaderStages shaderStages) override;
    virtual void SetSkybox(AssetID id) override;

    backend(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback);

    void StartResize() override;
    void EndResize(Fleur::SRect& rect) override;

    virtual void CreatePass(EFLPassKind kind, SFLShaderStages shaderStages) override;

    virtual void RegisterModel(const SFLModelRegisterInfo& info) override;
    virtual void UnregisterModel(AssetID model) override;
    virtual void RemoveTexture(AssetID texture) override;

    virtual void BeginFrame(SFLCameraData& cameraData) override;
    virtual void Draw(AssetID model, const glm::mat4& transform) override;
    virtual void EndFrame() override;

    virtual void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color, bool depthTest = true) override;
    virtual void DrawPoint(glm::vec3 p, glm::vec3 color, float size = 4.0f, bool depthTest = true) override;

    virtual void SetDirectionalLight(glm::vec3 direction, glm::vec4 color, float intensity) override;
    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) override;

private:
    impl* pImpl;
};
}  // namespace vk