#include "SwapchainOpenGL.h"

#include <glad/wgl.h>

#include "OpenGL/SurfaceOpenGL.h"

namespace Fleur::Graphics
{
SwapchainOpenGL::SwapchainOpenGL(std::unique_ptr<Surface> surface)
{
    auto rawSurface = static_cast<SurfaceOpenGL*>(surface.release());
    this->m_Surface = std::unique_ptr<SurfaceOpenGL>(rawSurface);

    auto rect = this->m_Surface->GetRect();

    m_BackBuffer = std::make_unique<DefaultFramebufferOpenGL>(rect.width, rect.height);

    glViewport(rect.x, rect.y, rect.width, rect.height);
}

SwapchainOpenGL::~SwapchainOpenGL()
{
}

void SwapchainOpenGL::Present()
{
    m_BackBuffer->Bind();
    SwapBuffers(m_Surface->GetHdc());
}

const Framebuffer& SwapchainOpenGL::GetScreenTexture()
{
    return *m_BackBuffer.get();
}

void SwapchainOpenGL::ShowWireFrame(bool show)
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

void SwapchainOpenGL::ValidateWindow()
{
    static PAINTSTRUCT ps;
    BeginPaint((HWND)m_Surface->GetNativeHandle(), &ps);
    EndPaint((HWND)m_Surface->GetNativeHandle(), &ps);
}

void SwapchainOpenGL::ClearBackbuffer() const
{
    m_BackBuffer->Clear();
}
void SwapchainOpenGL::Release()
{
    m_Surface->Release();
}
}  // namespace Fleur::Graphics
