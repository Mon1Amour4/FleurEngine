#include "InputLinux.h"

#include "Application.h"
#include "WindowLinux.h"

template <>
Fleur::InputLinux& Fleur::singleton<Fleur::InputLinux>::instance()
{
    static InputLinux inst;
    return inst;
}

bool Fleur::InputLinux::IsKeyPressedImpl(EKeyCode keyCode) const
{
    const WindowLinux& window = static_cast<const WindowLinux&>(Application::instance().GetWindow());
    Input::EKeyState state = window.GetKeyState(keyCode);
    return state == Input::KEY_PRESSED || state == Input::KEY_REPEAT;
}

bool Fleur::InputLinux::IsMouseButtonPressedImpl(uint16_t mouseCode)
{
    const WindowLinux& window = static_cast<const WindowLinux&>(Application::instance().GetWindow());
    Input::EMouseState state = window.GetMouseState(mouseCode);
    return state == Input::MOUSE_LPRESSED || state == Input::MOUSE_RPRESSED;
}

std::pair<int, int> Fleur::InputLinux::GetMousePositionImpl() const
{
    const WindowLinux& window = static_cast<const WindowLinux&>(Application::instance().GetWindow());
    int xPos, yPos;
    window.GetMousePos(xPos, yPos);
    return {xPos, yPos};
}

bool Fleur::InputLinux::IsMouseWheelScrolledImpl(std::pair<int, int>& pair) const
{
    const WindowLinux& window = static_cast<const WindowLinux&>(Application::instance().GetWindow());
    pair = window.GetMouseWheelScrollData();
    if (pair.first != 0.f || pair.second != 0.f)
        return true;
    return false;
}

int Fleur::InputLinux::GetMouseXImpl() const
{
    auto [x, y] = GetMousePositionImpl();
    return x;
}

int Fleur::InputLinux::GetMouseYImpl() const
{
    auto [x, y] = GetMousePositionImpl();
    return y;
}

glm::ivec2 Fleur::InputLinux::GetMouseDirImpl() const
{
    const WindowLinux& window = static_cast<const WindowLinux&>(Application::instance().GetWindow());
    return window.GetMouseDir();
}

Fleur::Input& Fleur::Input::platform_instance()
{
    return InputLinux::instance();
}
