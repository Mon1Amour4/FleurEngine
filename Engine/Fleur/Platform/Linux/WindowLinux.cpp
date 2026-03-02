#include "WindowLinux.h"

#include "Log.h"
#include "Wayland.h"

void Fleur::WindowLinux::SetTitle(std::string title)
{
    m_Wayland_window->setTitle(std::string(m_Props.Title + " " + title).c_str());
}

Fleur::SRect Fleur::WindowLinux::GetFramebufferSize() const
{
    return {0, 0, m_Wayland_window->width, m_Wayland_window->height};
}

Fleur::WindowLinux::WindowLinux(const WindowProps& props, EventQueue& eventQueue)
    : m_EventQueue(static_cast<EventQueueLinux*>(&eventQueue))
    , m_Props(props)
    , m_LastMouse{Input::MOUSE_NONE, Mouse::None}
    , m_CursorPos{0.f, 0.f}
    , m_IsResizing(false)
    , m_IsPainted(true)
    , m_IsFrameAction(false)
    , m_CurrentWidth(0)
    , m_CurrentHeigth(0)
    , m_XPos(props.x)
    , m_YPos(props.y)
    , m_PrevCursorPos(m_CursorPos)
    , m_InteractionMode(EInteractionMode::GAMING)
    , m_IsFirstLaunch(true)
    , m_IsAppActive(false)
    , m_BufferX(0)
    , m_BufferY(0)
    , m_PrevMouseDir(0, 0)
    , m_MouseDir(0, 0)
    , m_MouseWheelData(std::make_pair(0, 0))
{
    m_Wayland_ctx = std::make_shared<Wayland::Context>();
    m_Wayland_window = std::make_shared<Wayland::Window>();
    if (!m_Wayland_ctx->Open(m_EventQueue) or !m_Wayland_window->Open(m_Wayland_ctx))
        throw std::runtime_error("failed to create Wayland window");

    m_Wayland_window->flush();

    m_Wayland_handle.display = m_Wayland_ctx->display.get();
    m_Wayland_handle.surface = m_Wayland_window->m_surface.get();

    m_DPIScale = 96.0f / 96.0f;
}

void Fleur::WindowLinux::OnUpdate(float dtTime)
{
    m_Wayland_ctx->poll();

    // TODO thread
    // std::atomic<bool> stopFlag = true;
    // m_Wayland_ctx->eventLoop(&stopFlag);

    UNUSED(dtTime);
    glm::ivec2 tmp = m_MouseDir;
    m_MouseDir.x = static_cast<int>(std::lerp(m_PrevMouseDir.x, m_BufferX, 0.5f));
    m_MouseDir.y = static_cast<int>(std::lerp(m_PrevMouseDir.y, m_BufferY, 0.5f));
    m_PrevMouseDir = tmp;

    m_BufferX = 0;
    m_BufferY = 0;
    m_EventQueue->PushEvent(std::make_shared<EventVariant>(AppRenderEvent()));
}

void Fleur::WindowLinux::OnPostUpdate(float dtTime)
{
    UNUSED(dtTime);
}

void Fleur::WindowLinux::OnFixedUpdate()
{
}

void* Fleur::WindowLinux::GetNativeHandle() const
{
    return const_cast<void*>(reinterpret_cast<const void*>(&m_Wayland_handle));
}

Fleur::Input::EKeyState Fleur::WindowLinux::GetKeyState(EKeyCode keyCode) const
{
    return m_Wayland_ctx->key_states[keyCode];
}

Fleur::Input::EMouseState Fleur::WindowLinux::GetMouseState(EMouseCode mouseCode) const
{
    return m_LastMouse.MouseCode == mouseCode ? m_LastMouse.State : Input::EMouseState::MOUSE_NONE;
}

std::pair<int, int> Fleur::WindowLinux::GetMouseWheelScrollData() const
{
    return m_MouseWheelData;
}

void Fleur::WindowLinux::GetMousePos(int& xPos, int& yPos) const
{
    xPos = m_CursorPos.x;
    yPos = m_CursorPos.y;
}

std::unique_ptr<Fleur::Window> Fleur::Window::CreateAppWindow(const WindowProps& props, EventQueue& eventQueue)
{
    return std::make_unique<WindowLinux>(props, eventQueue);
}

void Fleur::WindowLinux::SetMousePos(int x, int y)
{
    m_PrevCursorPos = m_CursorPos;
    m_CursorPos.x = x;
    m_CursorPos.y = y;
}

void Fleur::WindowLinux::SetMouseWheelScrollData(int x, int y)
{
    m_MouseWheelData.first = x;
    m_MouseWheelData.second = y;
}

void Fleur::WindowLinux::SetGamingMode()
{
}

void Fleur::WindowLinux::UnlockMouse()
{
}
