#pragma once

#include <cassert>

#include "IRenderer.hpp"
#include "Renderer_Vulkan.h"

struct vulkanBackend : public IRenderer
{
    // pImpl
    struct vulkanBackendImpl;

    // Vrtual interface
    virtual ~vulkanBackend() override;
    virtual void AddToDrawList(SFLDrawUploadInfo* pInfo) override;
    virtual void Update(float dtTime) override;

    vulkanBackend(void* pNativeHandle, Fleur::SRect framebufferSize);

    void ResizeEvent(Fleur::SRect& rect);

private:
    vulkanBackendImpl* pImpl;
};
