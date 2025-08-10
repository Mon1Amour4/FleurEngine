#include "SwapchainOpenGL.h"

#include "OpenGL/SurfaceOpenGL.h"
// clang-format off
#include "glad/wgl.h"
#include "glad/gl.h"
// clang-format on

namespace Fuego::Graphics
{
SwapchainOpenGL::SwapchainOpenGL(std::unique_ptr<Surface> _surface)
{
    auto raw_surface = static_cast<SurfaceOpenGL*>(_surface.release());
    this->_surface = std::unique_ptr<SurfaceOpenGL>(raw_surface);

    auto rect = this->_surface->GetRect();

    this->_backbuffer = std::make_unique<DefaultFramebufferOpenGL>(rect.width, rect.height);

    glViewport(rect.x, rect.y, rect.width, rect.height);
}

SwapchainOpenGL::~SwapchainOpenGL()
{
}

void SwapchainOpenGL::Present()
{
    SwapBuffers(_surface->GetHdc());
}

const Framebuffer& SwapchainOpenGL::GetScreenTexture()
{
    return *_backbuffer.get();
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
    BeginPaint((HWND)_surface->GetNativeHandle(), &ps);
    EndPaint((HWND)_surface->GetNativeHandle(), &ps);
}

void SwapchainOpenGL::UpdateVieport()
{
    auto rect = _surface->GetRect();

    _backbuffer->ResizeFBO(rect.width, rect.height);

    glViewport(rect.x, rect.y, rect.width, rect.height);
}
void SwapchainOpenGL::ClearBackbuffer() const
{
    _backbuffer->Bind();
    _backbuffer->Clear();
}
void SwapchainOpenGL::Release()
{
    _surface->Release();
    _backbuffer->Release();
}
}  // namespace Fuego::Graphics
