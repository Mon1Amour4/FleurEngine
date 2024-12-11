#include "SurfaceOpenGL.h"

namespace Fuego::Renderer
{
SurfaceOpenGL::SurfaceOpenGL(const void* window)
{
    _window = (HWND)window;
    _hdc = GetDC(_window);
    _pfd = {sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            32,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            24,
            8,
            0,
            PFD_MAIN_PLANE,
            0,
            0,
            0,
            0};
}

SurfaceOpenGL::~SurfaceOpenGL()
{
    ReleaseDC(_window, _hdc);
}

const void* SurfaceOpenGL::GetNativeHandle() const
{
    return _window;
}
HDC SurfaceOpenGL::GetHdc() const
{
    return _hdc;
}

const PIXELFORMATDESCRIPTOR* SurfaceOpenGL::GetPfd() const
{
    return &_pfd;
}

}  // namespace Fuego::Renderer
