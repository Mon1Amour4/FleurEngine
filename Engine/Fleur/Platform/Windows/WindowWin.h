#pragma once

#include "EventQueueWin.h"
#include "Input.h"
#include "Window.h"

namespace Fleur
{
class SurfaceWindows;

class WindowWin final : public Window
{
public:
    friend class Application;

    WindowWin(const WindowProps& props, EventQueue& eventQueue);

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

    virtual const void* GetNativeHandle() const override;

    LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

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
    inline virtual EInteractionMode GetInteractionMode() const
    {
        return m_InteractionMode;
    }

    virtual inline bool IsResizing() const override
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

private:
    uint32_t m_CurrentWidth, m_CurrentHeigth;
    int m_XPos, m_YPos;

    static DWORD WinThreadMain(_In_ LPVOID lpParameter);
    static LRESULT CALLBACK WindowProcStatic(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void InitOpenGLExtensions();

    EventQueueWin* m_EventQueue;

    // Window handle
    HWND m_HWND;
    HINSTANCE m_Hinstance;  // Relates to the Application
    WindowProps m_Props;

    // Threads
    HANDLE m_WinThread;
    LPDWORD m_WinThreadID;
    HANDLE m_OnThreadCreated;

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
    // Raw Input Device
    RAWINPUTDEVICE Rid[2];

protected:
    virtual void SetWindowMode(WPARAM mode);
};

}  // namespace Fleur
