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
    virtual bool IsMouseWheelScrolledImpl(std::pair<float, float>& pair) const override;
    virtual std::pair<float, float> GetMousePositionImpl() const override;
    virtual float GetMouseXImpl() const override;
    virtual float GetMouseYImpl() const override;
    virtual glm::vec2 GetMouseDirImpl() const override;

    Input::KeyInfo m_LastKey;
    Input::MouseInfo m_LastMouse;
};
}  // namespace Fleur
