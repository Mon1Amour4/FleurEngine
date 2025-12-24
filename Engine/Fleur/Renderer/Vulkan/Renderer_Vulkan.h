#pragma once

#include <Vulkan/vulkan.h>

#include <cassert>

#include "IRenderer.hpp"
#include "Renderer_Vulkan.h"

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
        void enableValidationLayersSupport(VkInstanceCreateInfo& createInfo, const char** layerNames, uint32_t layerCount);
        void enableExtensions(VkInstanceCreateInfo& createInfo, const char** extensions, uint32_t count);
    };
}