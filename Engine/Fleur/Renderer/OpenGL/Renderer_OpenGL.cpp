#include "Renderer_OpenGL.h"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason_for_call, LPVOID lpReserved)
{
    switch (reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        std::cout << "Renderer_OpenGLBackend: DLL_PROCESS_ATTACH";
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        std::cout << "Renderer_OpenGLBackend: DLL_PROCESS_DETACH";
        break;
    }
    case DLL_THREAD_ATTACH:
    {
        std::cout << "Renderer_OpenGLBackend: DLL_THREAD_ATTACH";
        break;
    }
    case DLL_THREAD_DETACH:
    {
        std::cout << "Renderer_OpenGLBackend: DLL_THREAD_DETACH";
        break;
    }
    }
    return TRUE;
}

RENDERER_BACKEND_EXPORT IRenderer* Renderer::Backend::CreateRendererBackend(void)
{
    return new RendererOpenGL();
}

RENDERER_BACKEND_EXPORT void Renderer::Backend::DestroyRendererBackend(IRenderer* backend)
{
    delete backend;
}

void Renderer::Backend::RendererOpenGL::Draw(DrawInfo info)
{
}
