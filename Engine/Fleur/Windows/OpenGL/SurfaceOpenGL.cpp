#include "SurfaceOpenGL.h"

#include <glad/gl.h>

namespace Fleur::Graphics
{
SurfaceOpenGL::SurfaceOpenGL(const void* window)
{
    m_Window = (HWND)window;
    m_HDC = GetDC(m_Window);
}

SurfaceOpenGL::~SurfaceOpenGL()
{
    Release();
}

Surface::Rect SurfaceOpenGL::GetRect() const
{
    RECT rect;
    if (GetClientRect(m_Window, &rect))
    {
        return {rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
    }

    return {0, 0, 0, 0};
}

const void* SurfaceOpenGL::GetNativeHandle() const
{
    return m_Window;
}

HDC SurfaceOpenGL::GetHdc() const
{
    return m_HDC;
}

void SurfaceOpenGL::Release()
{
    if (m_HDC)
        ReleaseDC(m_Window, m_HDC);
    m_HDC = nullptr;
}

}  // namespace Fleur::Graphics
