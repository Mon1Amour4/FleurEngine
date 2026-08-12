#pragma once

#include <cassert>

#include "IRenderer.hpp"

using namespace Fleur::Graphics;

namespace spdlog
{
class logger;
}

namespace gl
{
// OpenGL backend. Mirrors vk::backend: a thin public shim over a private impl,
// implementing the shared IRenderer contract (models / textures / skybox).
struct backend : public Fleur::Graphics::IRenderer
{
    struct impl;

    backend(bool enableValidation, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback, uint32_t maxPointLights,
            uint32_t cascadeCount, Fleur::Graphics::LightSampling directionalLight, Fleur::Graphics::LightSampling pointLight,
            std::shared_ptr<spdlog::logger> logger);
    virtual ~backend() override;

    void CreatePass(EFLPassKind kind, SFLShaderStages shaderStages) override;
    void CreateShadowPass(EFLShadowPassKind kind, SFLShaderStages shaderStages) override;
    void ConfigureDebugDraw(const SFLDebugDrawShaders& shaders) override;
    void ConfigureOverlay(SFLShaderStages shaderStages) override;
    void CreateSkybox(AssetID id, SFLShaderStages shaderStages) override;
    void SetSkybox(AssetID id) override;
    void CreateFloor(AssetID texture, SFLShaderStages shaderStages, float height = 0.0f) override;
    void SetFloor(AssetID texture, float height) override;

    void RegisterModel(const SFLModelRegisterInfo& info) override;
    void UnregisterModel(AssetID model) override;
    void UploadTextures(SFLImageViewInfo* pInfo) override;
    void RemoveTexture(AssetID texture) override;

    void BeginFrame(const RenderFrameData& frameData) override;
    void Draw(AssetID model, const Fleur::Mat4& transform) override;
    void EndFrame() override;

    void StartResize() override;
    void EndResize(Fleur::SRect& rect) override;
    void SetShaderRegistry(const ShaderRegistry& shaders) override;
    void SetShadowSceneBounds(const Fleur::Graphics::BoundingBox& bounds) override;
    void SetShadowSettings(uint32_t, Fleur::Graphics::LightSampling, Fleur::Graphics::LightSampling) override {};

    void DrawLine(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 color, bool depthTest = true) override {};  // TODO: GlDebugDraw
    void DrawPoint(Fleur::Vec3 p, Fleur::Vec3 color, float size = 4.0f, bool depthTest = true) override {};
    virtual void DrawQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, Fleur::Vec4 color, bool depthTest = true) {};   // TODO
    virtual void DrawQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, uint32_t texture, bool depthTest = true) {};  // TODO
    virtual void DrawBillboard(Fleur::Vec3 center, Fleur::Vec2 size, uint32_t texture, bool depthTest = true) override {};      // TODO
    virtual void DrawOverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, Fleur::Vec4 color) override {};
    virtual void DrawOverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, uint32_t texture) override {};
    virtual void DrawOverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec4 color) override {};
    virtual void DrawOverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, uint32_t texture) override {};
    virtual void DrawShadowMapOverlay(Fleur::Vec2 min, Fleur::Vec2 max, int32_t layer) override {};

    virtual void SetDirectionalLight(Fleur::Vec3 direction, Fleur::Vec4 color, float intensity) {};  // TODO: DirectionalLight
    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) override {};  // TODO: PointLight

private:
    impl* pImpl;
};
}  // namespace gl
