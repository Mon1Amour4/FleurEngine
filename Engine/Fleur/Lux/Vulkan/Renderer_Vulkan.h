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
    virtual void ConfigureDebugDraw(const SFLDebugDrawShaders& shaders) override;
    virtual void ConfigureOverlay(SFLShaderStages shaderStages) override;

    virtual void RegisterModel(const SFLModelRegisterInfo& info) override;
    virtual void UnregisterModel(AssetID model) override;
    virtual void RemoveTexture(AssetID texture) override;

    virtual void BeginFrame(const Fleur::Graphics::RenderFrameData& frameData) override;
    virtual void Draw(AssetID model, const glm::mat4& transform) override;
    virtual void EndFrame() override;

    virtual void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color, bool depthTest = true) override;
    virtual void DrawPoint(glm::vec3 p, glm::vec3 color, float size = 4.0f, bool depthTest = true) override;
    virtual void DrawQuad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec4 color, bool depthTest = true) override;
    virtual void DrawQuad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, uint32_t texture, bool depthTest = true) override;
    virtual void DrawBillboard(glm::vec3 center, glm::vec2 size, uint32_t texture, bool depthTest = true) override;
    virtual void DrawOverlayQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, glm::vec4 color) override;
    virtual void DrawOverlayQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, uint32_t texture) override;
    virtual void DrawOverlayTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 color) override;
    virtual void DrawOverlayTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, uint32_t texture) override;
    virtual void DrawShadowMapOverlay(glm::vec2 min, glm::vec2 max) override;

    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) override;

private:
    impl* pImpl;
};
}  // namespace vk
