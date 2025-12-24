#pragma once

#include <windows.h>
#include <iostream>

#include "../IRenderer.hpp"

#if defined(DLL_EXPORT)
#define RENDERER_BACKEND_EXPORT __declspec(dllexport)
#else 
    #define RENDERER_BACKEND_EXPORT __declspec(dllimport)
#endif

namespace Renderer::Backend
{
extern "C"
{
    RENDERER_BACKEND_EXPORT IRenderer* CreateRendererBackend(void);
    RENDERER_BACKEND_EXPORT void DestroyRendererBackend(IRenderer* backend);

    class RendererOpenGL : public IRenderer
    {
        virtual void Draw(DrawInfo info) override;
    };
}
}  // namespace Renderer::Backend