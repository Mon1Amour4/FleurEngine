#include "SurfaceOpenGL.h"

#include <glad/gl.h>

namespace Fleur::Graphics
{
SurfaceOpenGL::SurfaceOpenGL(const void* window)
{
    _window = (HWND)window;
    _hdc = GetDC(_window);
}

SurfaceOpenGL::~SurfaceOpenGL()
{
    Release();
}

Surface::Rect SurfaceOpenGL::GetRect() const
{
    RECT rect;
    if (GetClientRect(_window, &rect))
    {
        return {rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
    }

    return {0, 0, 0, 0};
}

const void* SurfaceOpenGL::GetNativeHandle() const
{
    return _window;
}

HDC SurfaceOpenGL::GetHdc() const
{
    return _hdc;
}

void SurfaceOpenGL::Release()
{
    if (_hdc)
        ReleaseDC(_window, _hdc);
    _hdc = nullptr;
}

}  // namespace Fleur::Graphics
