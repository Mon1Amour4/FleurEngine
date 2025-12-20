#include "SurfaceOpenGL.h"

#include <glad/wgl.h>

Fleur::Graphics::SurfaceOpenGL::SurfaceOpenGL(const void* window)
{
    m_Window = (HWND)window;
    m_HDC = GetDC(m_Window);
}

Fleur::Graphics::SurfaceOpenGL::~SurfaceOpenGL()
{
    Release();
}

Fleur::Graphics::Rect Fleur::Graphics::SurfaceOpenGL::GetRect() const
{
    RECT rect;
    if (GetClientRect(m_Window, &rect))
    {
        return {rect.left, rect.top, static_cast<uint32_t>(rect.right - rect.left), static_cast<uint32_t>(rect.bottom - rect.top)};
    }

    return {0, 0, 0, 0};
}

const void* Fleur::Graphics::SurfaceOpenGL::GetNativeHandle() const
{
    return m_Window;
}

HDC Fleur::Graphics::SurfaceOpenGL::GetHdc() const
{
    return m_HDC;
}

void Fleur::Graphics::SurfaceOpenGL::Release()
{
    if (m_HDC)
        ReleaseDC(m_Window, m_HDC);
    m_HDC = nullptr;
}
