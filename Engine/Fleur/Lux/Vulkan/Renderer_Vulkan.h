#pragma once

#include <cassert>
#include <memory>

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
    virtual void CreateFloor(AssetID texture, SFLShaderStages shaderStages, float height = 0.0f) override;
    virtual void SetFloor(AssetID texture, float height) override;

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
    virtual void Draw(AssetID model, const Fleur::Mat4& transform) override;
    virtual void EndFrame() override;

    virtual void DrawLine(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 color, bool depthTest = true) override;
    virtual void DrawPoint(Fleur::Vec3 p, Fleur::Vec3 color, float size = 4.0f, bool depthTest = true) override;
    virtual void DrawQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, Fleur::Vec4 color, bool depthTest = true) override;
    virtual void DrawQuad(Fleur::Vec3 a, Fleur::Vec3 b, Fleur::Vec3 c, Fleur::Vec3 d, uint32_t texture, bool depthTest = true) override;
    virtual void DrawBillboard(Fleur::Vec3 center, Fleur::Vec2 size, uint32_t texture, bool depthTest = true) override;
    virtual void DrawOverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, Fleur::Vec4 color) override;
    virtual void DrawOverlayQuad(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec2 d, uint32_t texture) override;
    virtual void DrawOverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, Fleur::Vec4 color) override;
    virtual void DrawOverlayTriangle(Fleur::Vec2 a, Fleur::Vec2 b, Fleur::Vec2 c, uint32_t texture) override;
    virtual void DrawShadowMapOverlay(Fleur::Vec2 min, Fleur::Vec2 max) override;

    virtual void UpdatePointLight(const SFLPointLight* light, uint32_t lightCount) override;

private:
    std::unique_ptr<impl> pImpl;
};
}  // namespace vk
