#pragma once

#include "../Windows/EventQueueWin.h"
#include "Input.h"
#include "Window.h"

namespace Fleur
{
class SurfaceWindows;

class WindowLinux final : public Window
{
public:
    friend class Application;

    WindowLinux(const WindowProps& props, EventQueue& eventQueue);

    virtual void OnUpdate(float dtTime) override;
    virtual void OnPostUpdate(float dtTime) override;
    virtual void OnFixedUpdate() override;

    inline virtual unsigned int GetWidth() const override
    {
        return m_Props.Width;
    }
    inline virtual unsigned int GetHeight() const override
    {
        return m_Props.Height;
    }

    virtual void* GetNativeHandle() const override;

    Input::EKeyState GetKeyState(EKeyCode keyCode) const;
    Input::EMouseState GetMouseState(EMouseCode mouseCode) const;
    void GetMousePos(int& xPos, int& yPos) const;
    std::pair<int, int> GetMouseWheelScrollData() const;
    virtual inline bool HasMouseMoved(int x, int y) const override
    {
        return !(m_CursorPos.x == x && m_CursorPos.y == y);
    }

    inline virtual void SwitchInteractionMode() override
    {
        m_InteractionMode = m_InteractionMode == EInteractionMode::GAMING ? EInteractionMode::EDITOR : EInteractionMode::GAMING;
        if (m_InteractionMode == EInteractionMode::GAMING)
        {
            SetGamingMode();
        }
    }
    inline virtual EInteractionMode GetInteractionMode() const override
    {
        return m_InteractionMode;
    }

    inline virtual bool IsResizing() const override
    {
        return m_IsResizing;
    }
    inline virtual glm::vec2 GetMouseDir() const override
    {
        return m_MouseDir;
    }

    inline virtual bool IsActive() const override
    {
        return m_HasInputFocus;
    }

    virtual void SetTitle(std::string title) override;

    virtual Fleur::SRect GetFramebufferSize() const override;

private:

    EventQueueWin* m_EventQueue;

    uint32_t m_CurrentWidth, m_CurrentHeigth;
    int m_XPos, m_YPos;


    WindowProps m_Props;

    bool m_IsFirstLaunch, m_IsResizing, m_IsPainted, m_HasInputFocus, m_IsAppActive, m_IsFrameAction;

    virtual inline void SetPainted() override
    {
        m_IsPainted = true;
    }

    virtual void SetMousePos(int x, int y) override;
    virtual void SetMouseWheelScrollData(int x, int y) override;


    glm::ivec2 m_MouseDir;
    glm::ivec2 m_PrevMouseDir;
    Input::MouseInfo m_LastMouse;
    Input::EKeyState m_PressedKeys[256];
    glm::ivec2 m_CursorPos;
    glm::ivec2 m_PrevCursorPos;

    int m_BufferX, m_BufferY = 0;
    std::pair<int, int> m_MouseWheelData;

    EInteractionMode m_InteractionMode;
    void SetGamingMode();
    void UnlockMouse();
};

}  // namespace Fleur
