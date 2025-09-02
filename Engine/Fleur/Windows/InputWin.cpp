#include "InputWin.h"

#include "Application.h"
#include "WindowWin.h"

namespace Fleur
{
template <>
InputWin& singleton<InputWin>::instance()
{
    static InputWin inst;
    return inst;
}

bool InputWin::IsKeyPressedImpl(EKeyCode keyCode) const
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    Input::EKeyState state = window.GetKeyState(keyCode);
    return state == Input::KEY_PRESSED || state == Input::KEY_REPEAT;
}

bool InputWin::IsMouseButtonPressedImpl(uint16_t mouseCode)
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    Input::EMouseState state = window.GetMouseState(mouseCode);
    return state == Input::MOUSE_LPRESSED || state == Input::MOUSE_RPRESSED;
}

std::pair<int, int> InputWin::GetMousePositionImpl() const
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    int xPos, yPos;
    window.GetMousePos(xPos, yPos);
    return {xPos, yPos};
}

bool InputWin::IsMouseWheelScrolledImpl(std::pair<int, int>& pair) const
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    pair = window.GetMouseWheelScrollData();
    if (pair.first != 0.f || pair.second != 0.f)
        return true;
    return false;
}

int InputWin::GetMouseXImpl() const
{
    auto [x, y] = GetMousePositionImpl();
    return x;
}

int InputWin::GetMouseYImpl() const
{
    auto [x, y] = GetMousePositionImpl();
    return y;
}

glm::ivec2 InputWin::GetMouseDirImpl() const
{
    const WindowWin& window = static_cast<const WindowWin&>(Application::instance().GetWindow());
    return window.GetMouseDir();
}

Input& Input::platform_instance()
{
    return InputWin::instance();
}
}  // namespace Fleur
