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
    void Draw(AssetID model, const glm::mat4& transform) override;
    void EndFrame() override;

    void StartResize() override;
    void EndResize(Fleur::SRect& rect) override;

    void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color, bool depthTest = true) override {};  // TODO: GlDebugDraw
    void DrawPoint(glm::vec3 p, glm::vec3 color, float size = 4.0f, bool depthTest = true) override {};
    virtual void DrawQuad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec4 color, bool depthTest = true) {};   // TODO
    virtual void DrawQuad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, uint32_t texture, bool depthTest = true) {};  // TODO
    virtual void DrawBillboard(glm::vec3 center, glm::vec2 size, uint32_t texture, bool depthTest = true) override {};      // TODO
    virtual void DrawOverlayQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, glm::vec4 color) override {};
    virtual void DrawOverlayQuad(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d, uint32_t texture) override {};
    virtual void DrawOverlayTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 color) override {};
    virtual void DrawOverlayTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, uint32_t texture) override {};
    virtual void DrawShadowMapOverlay(glm::vec2 min, glm::vec2 max) override {};

    virtual void SetDirectionalLight(glm::vec3 direction, glm::vec4 color, float intensity) {};  // TODO: DirectionalLight
    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) override {};  // TODO: PointLight

private:
    impl* pImpl;
};
}  // namespace gl
