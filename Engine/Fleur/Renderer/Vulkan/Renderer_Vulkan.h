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
    virtual void Draw(DrawInfo info) override;

    vulkanBackend(void* pNativeHandle);

private:
    vulkanBackendImpl* pImpl;
};
