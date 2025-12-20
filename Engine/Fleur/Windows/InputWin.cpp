#include "InputWin.h"

#include "Application.h"
#include "WindowWin.h"

template <>
Fleur::InputWin& Fleur::singleton<Fleur::InputWin>::instance()
{
    static InputWin inst;
    return inst;
}

bool Fleur::InputWin::IsKeyPressedImpl(EKeyCode keyCode) const
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    Input::EKeyState state = window.GetKeyState(keyCode);
    return state == Input::KEY_PRESSED || state == Input::KEY_REPEAT;
}

bool Fleur::InputWin::IsMouseButtonPressedImpl(uint16_t mouseCode)
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    Input::EMouseState state = window.GetMouseState(mouseCode);
    return state == Input::MOUSE_LPRESSED || state == Input::MOUSE_RPRESSED;
}

std::pair<int, int> Fleur::InputWin::GetMousePositionImpl() const
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    int xPos, yPos;
    window.GetMousePos(xPos, yPos);
    return {xPos, yPos};
}

bool Fleur::InputWin::IsMouseWheelScrolledImpl(std::pair<int, int>& pair) const
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    pair = window.GetMouseWheelScrollData();
    if (pair.first != 0.f || pair.second != 0.f)
        return true;
    return false;
}

int Fleur::InputWin::GetMouseXImpl() const
{
    auto [x, y] = GetMousePositionImpl();
    return x;
}

int Fleur::InputWin::GetMouseYImpl() const
{
    auto [x, y] = GetMousePositionImpl();
    return y;
}

glm::ivec2 Fleur::InputWin::GetMouseDirImpl() const
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    return window.GetMouseDir();
}

Fleur::Input& Fleur::Input::platform_instance()
{
    return InputWin::instance();
}
