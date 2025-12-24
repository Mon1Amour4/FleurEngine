#include "SwapchainOpenGL.h"

#include <glad/wgl.h>

#include "OpenGL/SurfaceOpenGL.h"

Fleur::Graphics::SwapchainOpenGL::SwapchainOpenGL(std::unique_ptr<Surface> surface)
{
    auto rawSurface = static_cast<SurfaceOpenGL*>(surface.release());
    this->m_Surface = std::unique_ptr<SurfaceOpenGL>(rawSurface);

    Rect rect = this->m_Surface->GetRect();

    m_BackBuffer = std::make_unique<DefaultFramebufferOpenGL>(rect.width, rect.height);

    glViewport(rect.x, rect.y, rect.width, rect.height);
}

Fleur::Graphics::SwapchainOpenGL::~SwapchainOpenGL()
{
}

void Fleur::Graphics::SwapchainOpenGL::Present()
{
    m_BackBuffer->Bind();
    SwapBuffers(m_Surface->GetHdc());
}

const Fleur::Graphics::Framebuffer& Fleur::Graphics::SwapchainOpenGL::GetScreenTexture()
{
    return *m_BackBuffer.get();
}

void Fleur::Graphics::SwapchainOpenGL::ShowWireFrame(bool show)
{
    if (show)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Fleur::Graphics::SwapchainOpenGL::ValidateWindow()
{
    static PAINTSTRUCT ps;
    BeginPaint((HWND)m_Surface->GetNativeHandle(), &ps);
    EndPaint((HWND)m_Surface->GetNativeHandle(), &ps);
}

void Fleur::Graphics::SwapchainOpenGL::ClearBackbuffer() const
{
    m_BackBuffer->Clear();
}
void Fleur::Graphics::SwapchainOpenGL::Release()
{
    m_Surface->Release();
}
