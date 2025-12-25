#pragma once

#include "Input.h"
#include "singleton.hpp"

namespace Fleur
{
class InputWin final : public Input, public singleton<InputWin>
{
    friend class singleton<InputWin>;

protected:
    virtual bool IsKeyPressedImpl(EKeyCode keyCode) const override;

    virtual bool IsMouseButtonPressedImpl(EMouseCode mouseCode) override;
    virtual bool IsMouseWheelScrolledImpl(std::pair<int, int>& pair) const override;
    virtual std::pair<int, int> GetMousePositionImpl() const override;
    virtual int GetMouseXImpl() const override;
    virtual int GetMouseYImpl() const override;
    virtual glm::ivec2 GetMouseDirImpl() const override;

    Input::KeyInfo m_LastKey;
    Input::MouseInfo m_LastMouse;
};
}  // namespace Fleur
