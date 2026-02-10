#pragma once

#include <cassert>

#include "IRenderer.hpp"

using namespace Fleur::Graphics;

struct vulkanBackend : public Fleur::Graphics::IRenderer
{
    // pImpl
    struct vulkanBackendImpl;

    // Vrtual interface
    virtual ~vulkanBackend() override;
    virtual void AddToDrawList(SFLModelView* pModelView) override;
    virtual void Update(SFLGeometryUBO* pUbo) override;
    virtual void SubmitImageViews(SFLImageViewInfo* pInfo) override;

    virtual void CreateSkybox(AssetID id, SFLShaderInfo* pVertexShaderInfo, SFLShaderInfo* pFragmentShaderInfo) override;
    virtual void SetSkybox(AssetID id) override;

    vulkanBackend(bool enableValidation, SFLFrame& pFrame, void* pNativeHandle, Fleur::SRect& framebufferSize, SFLImageView& fallback);

    void StartResize() override;
    void EndResize(Fleur::SRect& rect) override;

private:
    vulkanBackendImpl* pImpl;
};
