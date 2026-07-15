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
    void ConfigureDebugDraw(const SFLDebugDrawShaders& shaders) override;
    void ConfigureOverlay(SFLShaderStages shaderStages) override;
    void CreateSkybox(AssetID id, SFLShaderStages shaderStages) override;
    void SetSkybox(AssetID id) override;

    void RegisterModel(const SFLModelRegisterInfo& info) override;
    void UnregisterModel(AssetID model) override;
    void UploadTextures(SFLImageViewInfo* pInfo) override;
    void RemoveTexture(AssetID texture) override;

    void BeginFrame(const RenderFrameData& frameData) override;
    void Draw(AssetID model, const Fleur::Math::mat4& transform) override;
    void EndFrame() override;

    void StartResize() override;
    void EndResize(Fleur::SRect& rect) override;

    void DrawLine(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 color, bool depthTest = true) override {};  // TODO: GlDebugDraw
    void DrawPoint(Fleur::Math::vec3 p, Fleur::Math::vec3 color, float size = 4.0f, bool depthTest = true) override {};
    virtual void DrawQuad(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 c, Fleur::Math::vec3 d, Fleur::Math::vec4 color, bool depthTest = true) {};   // TODO
    virtual void DrawQuad(Fleur::Math::vec3 a, Fleur::Math::vec3 b, Fleur::Math::vec3 c, Fleur::Math::vec3 d, uint32_t texture, bool depthTest = true) {};  // TODO
    virtual void DrawBillboard(Fleur::Math::vec3 center, Fleur::Math::vec2 size, uint32_t texture, bool depthTest = true) override {};      // TODO
    virtual void DrawOverlayQuad(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec2 d, Fleur::Math::vec4 color) override {};
    virtual void DrawOverlayQuad(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec2 d, uint32_t texture) override {};
    virtual void DrawOverlayTriangle(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, Fleur::Math::vec4 color) override {};
    virtual void DrawOverlayTriangle(Fleur::Math::vec2 a, Fleur::Math::vec2 b, Fleur::Math::vec2 c, uint32_t texture) override {};
    virtual void DrawShadowMapOverlay(Fleur::Math::vec2 min, Fleur::Math::vec2 max) override {};

    virtual void SetDirectionalLight(Fleur::Math::vec3 direction, Fleur::Math::vec4 color, float intensity) {};  // TODO: DirectionalLight
    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) override {};  // TODO: PointLight

private:
    impl* pImpl;
};
}  // namespace gl
