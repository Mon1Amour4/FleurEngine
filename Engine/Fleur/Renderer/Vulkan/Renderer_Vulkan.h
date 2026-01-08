#pragma once

#include <cassert>

#include "IRenderer.hpp"
#include "Renderer_Vulkan.h"

struct vulkanBackend : public Fleur::Graphics::IRenderer
{
    // pImpl
    struct vulkanBackendImpl;

    // Vrtual interface
    virtual ~vulkanBackend() override;
    virtual void AddToDrawList(Fleur::Graphics::SFLModelView* pModelView) override;
    virtual void Update(Fleur::Graphics::SFLGeometryUBO* pUbo) override;
    virtual void SubmitImageViews(Fleur::Graphics::SFLImageViewInfo* pInfo) override;

    vulkanBackend(Fleur::Graphics::SFLFrame* pFrame, void* pNativeHandle, Fleur::SRect framebufferSize);

    void ResizeEvent(Fleur::SRect& rect);

private:
    vulkanBackendImpl* pImpl;
};
