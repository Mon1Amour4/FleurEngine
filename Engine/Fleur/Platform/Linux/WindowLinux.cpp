#include "WindowLinux.h"

#include "InputLinux.h"
#include "Log.h"

void Fleur::WindowLinux::SetTitle(std::string title)
{
    UNUSED(title);
}

Fleur::SRect Fleur::WindowLinux::GetFramebufferSize() const
{
    return {0, 0, 0, 0};
}

Fleur::WindowLinux::WindowLinux(const WindowProps& props, EventQueue& eventQueue)
    : m_EventQueue(static_cast<EventQueueWin*>(&eventQueue))
    , m_Props(props)
    , m_LastMouse{Input::MOUSE_NONE, Mouse::None}
    , m_CursorPos{0.f, 0.f}
    , m_IsResizing(false)
    , m_IsPainted(true)
    , m_IsFrameAction(false)
    , m_CurrentWidth(props.Width)
    , m_CurrentHeigth(props.Height)
    , m_XPos(props.x)
    , m_YPos(props.y)
    , m_PrevCursorPos(m_CursorPos)
    , m_PressedKeys{Input::EKeyState::KEY_NONE}
    , m_InteractionMode(EInteractionMode::GAMING)
    , m_IsFirstLaunch(true)
    , m_HasInputFocus(false)
    , m_IsAppActive(false)
    , m_BufferX(0)
    , m_BufferY(0)
    , m_PrevMouseDir(0, 0)
    , m_MouseDir(0, 0)
    , m_MouseWheelData(std::make_pair(0, 0))
{
}

void Fleur::WindowLinux::OnUpdate(float dtTime)
{
    UNUSED(dtTime);
    glm::ivec2 tmp = m_MouseDir;
    m_MouseDir.x = static_cast<int>(std::lerp(m_PrevMouseDir.x, m_BufferX, 0.5f));
    m_MouseDir.y = static_cast<int>(std::lerp(m_PrevMouseDir.y, m_BufferY, 0.5f));
    m_PrevMouseDir = tmp;

    m_BufferX = 0;
    m_BufferY = 0;
    if (!m_HasInputFocus)
        if (m_IsResizing || m_Props.mode == MINIMIZED)
        {
            FL_CORE_INFO("stop rendering");
            return;
        }
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
    throw std::runtime_error("TODO");
}

Fleur::Input::EKeyState Fleur::WindowLinux::GetKeyState(EKeyCode keyCode) const
{
    return m_PressedKeys[keyCode];
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
