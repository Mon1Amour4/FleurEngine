#pragma once

#include <cassert>

#include "IRenderer.hpp"

struct vulkanBackend : public Fleur::Graphics::IRenderer
{
    // pImpl
    struct vulkanBackendImpl;

    // Vrtual interface
    virtual ~vulkanBackend() override;
    virtual void AddToDrawList(Fleur::Graphics::SFLModelView* pModelView) override;
    virtual void Update(Fleur::Graphics::SFLGeometryUBO* pUbo) override;
    virtual void SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo) override;

    vulkanBackend(bool enableValidation, Fleur::Graphics::SFLFrame& pFrame, void* pNativeHandle, Fleur::SRect& framebufferSize,
                  Fleur::Graphics::SFLImageView& fallback);

    void StartResize() override;
    void EndResize(Fleur::SRect& rect) override;

private:
    vulkanBackendImpl* pImpl;
};
