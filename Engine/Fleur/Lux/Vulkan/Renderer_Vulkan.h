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
    virtual void Draw(AssetID model, const Fleur::Math::mat4& transform) override;
    virtual void EndFrame() override;

    virtual void DrawLine(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 color, bool depthTest = true) override;
    virtual void DrawPoint(Fleur::Math::vec3 p, Fleur::Math::vec3 color, float size = 4.0f, bool depthTest = true) override;
    virtual void DrawQuad(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 c, Fleur::Math::vec3 d, Fleur::Math::vec4 color, bool depthTest = true) override;
    virtual void DrawQuad(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 c, Fleur::Math::vec3 d, uint32_t texture, bool depthTest = true) override;
    virtual void DrawBillboard(Fleur::Math::vec3 center, Fleur::Math::vec2 size, uint32_t texture, bool depthTest = true) override;
    virtual void DrawOverlayQuad(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec2 d, Fleur::Math::vec4 color) override;
    virtual void DrawOverlayQuad(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec2 d, uint32_t texture) override;
    virtual void DrawOverlayTriangle(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec4 color) override;
    virtual void DrawOverlayTriangle(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, uint32_t texture) override;
    virtual void DrawShadowMapOverlay(Fleur::Math::vec2 min, Fleur::Math::vec2 max) override;

    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) override;

private:
    impl* pImpl;
};
}  // namespace vk
