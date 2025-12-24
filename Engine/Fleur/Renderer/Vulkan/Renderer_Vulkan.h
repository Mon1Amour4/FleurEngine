#pragma once

#include <Vulkan/vulkan.h>
#include <cassert>

#include "Renderer_Vulkan.h"
#include "IRenderer.hpp"

#if defined(DLL_EXPORT)
#define RENDERER_BACKEND_EXPORT __declspec(dllexport)
#else
#define RENDERER_BACKEND_EXPORT __declspec(dllimport)
#endif

extern "C"
{
    RENDERER_BACKEND_EXPORT IRenderer* CreateRendererBackend(void);
    RENDERER_BACKEND_EXPORT void DestroyRendererBackend(IRenderer* backend);

    struct vulkanBackend : public IRenderer
    {
        // Vrtual interface
        virtual ~vulkanBackend() override;
        virtual void Draw(DrawInfo info) override;

        vulkanBackend();
        VkInstance instance;
        VkInstance createInstance();
    };
}