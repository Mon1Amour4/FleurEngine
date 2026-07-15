#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"

namespace Fleur
{
class FLEUR_API Input
{
public:
    enum EKeyState
    {
        KEY_NONE = 0,
        KEY_PRESSED = 1,
        KEY_REPEAT = 2,
        KEY_RELEASED = 3
    };
    struct KeyInfo
    {
        EKeyState state;
        EKeyCode keyCode;
    };
    enum EMouseState
    {
        MOUSE_NONE,
        MOUSE_LPRESSED,
        MOUSE_RPRESSED,
        MOUSE_MPRESSED,
        MOUSE_SCROLL
    };

    static inline bool IsKeyPressed(EKeyCode keyCode)
    {
        return platform_instance().IsKeyPressedImpl(keyCode);
    }

    static inline bool IsMouseWheelScrolled(std::pair<int, int>& data)
    {
        return platform_instance().IsMouseWheelScrolledImpl(data);
    }

    static inline bool IsMouseButtonPressed(EMouseCode mouseCode)
    {
        return platform_instance().IsMouseButtonPressedImpl(mouseCode);
    }

    static inline int GetMouseX()
    {
        return platform_instance().GetMouseXImpl();
    }

    static inline int GetMouseY()
    {
        return platform_instance().GetMouseYImpl();
    }

    static inline std::pair<int, int> GetMousePosition()
    {
        return platform_instance().GetMousePositionImpl();
    }

    static inline Fleur::Math::ivec2 GetMouseDir()
    {
        return platform_instance().GetMouseDirImpl();
    }

    struct MouseInfo
    {
        EMouseState State;
        EMouseCode MouseCode;
    };

protected:
    [[nodiscard]] virtual bool IsKeyPressedImpl(EKeyCode keyCode) const = 0;

    virtual bool IsMouseButtonPressedImpl(EMouseCode mouseCode) = 0;
    [[nodiscard]] virtual bool IsMouseWheelScrolledImpl(std::pair<int, int>& pair) const = 0;
    [[nodiscard]] virtual std::pair<int, int> GetMousePositionImpl() const = 0;
    [[nodiscard]] virtual int GetMouseXImpl() const = 0;
    [[nodiscard]] virtual int GetMouseYImpl() const = 0;
    [[nodiscard]] virtual Fleur::Math::ivec2 GetMouseDirImpl() const = 0;

private:
    static Input& platform_instance();
};
}  // namespace Fleur
