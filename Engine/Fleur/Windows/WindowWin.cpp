#include "WindowWin.h"

#include <glad/wgl.h>

#include "InputWin.h"
#include "KeyCodesWin.h"
#include "Log.h"

namespace Fleur
{

void WindowWin::SetTitle(std::string title)
{
    SetWindowText(m_HWND, std::string(m_Props.Title + " " + title).c_str());
}

DWORD WINAPI WindowWin::WinThreadMain(LPVOID lpParameter)
{
    InitOpenGLExtensions();

    WindowWin* window = static_cast<WindowWin*>(lpParameter);

    static TCHAR buffer[32] = TEXT("");
#ifdef UNICODE
    MultiByteToWideChar(CP_UTF8, 0, props.Title.c_str(), -1, buffer, _countof(buffer));
#else
    FL_CORE_ASSERT(sprintf_s(buffer, window->m_Props.Title.c_str()), "")
#endif
    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpszClassName = window->m_Props.APP_WINDOW_CLASS_NAME;
    wndClass.hInstance = window->m_Hinstance;
    wndClass.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.lpfnWndProc = WindowProcStatic;

    RegisterClassEx(&wndClass);

    DWORD style = WS_OVERLAPPEDWINDOW | WS_SYSMENU;
    RECT rect;
    rect.left = window->m_Props.x;
    rect.top = window->m_Props.y;
    rect.right = rect.left + window->m_Props.Width;
    rect.bottom = rect.top + window->m_Props.Height;

    AdjustWindowRect(&rect, style, true);

    window->m_HWND = CreateWindowEx(0, window->m_Props.APP_WINDOW_CLASS_NAME, buffer, style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                                   nullptr, nullptr, window->m_Hinstance, nullptr);

    FL_CORE_ASSERT(window->m_HWND, "[AppWindow] hasn't been initialized!");

    // Associate this instance with the HWND
    SetWindowLongPtr(window->m_HWND, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

    ShowWindow(window->m_HWND, SW_SHOW);
    SetEvent(window->m_OnThreadCreated);

    // Generic Mouse
    window->Rid[0].usUsagePage = 0x01;
    window->Rid[0].usUsage = 0x02;
    window->Rid[0].dwFlags = RIDEV_INPUTSINK;
    window->Rid[0].hwndTarget = window->m_HWND;

    // Keyboard, ignores legacy keyboard
    window->Rid[1].usUsagePage = 0x01;
    window->Rid[1].usUsage = 0x06;
    window->Rid[1].dwFlags = RIDEV_NOLEGACY;
    window->Rid[1].hwndTarget = 0;

    if (RegisterRawInputDevices(window->Rid, 2, sizeof(Rid[0])) == FALSE)
    {
        DWORD error = GetLastError();
        if (error)
        {
            LPSTR buffer = nullptr;
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error,
                                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, NULL);

            std::string message(buffer, size);
            LocalFree(buffer);

            FL_CORE_ERROR("Raw Input Device registration failed, error code: {0}, message: {1}", error, message);
        }
    }

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(window->m_HWND);
    DestroyIcon(wndClass.hIcon);
    DestroyCursor(wndClass.hCursor);
    UnregisterClass(window->m_Props.APP_WINDOW_CLASS_NAME, window->m_Hinstance);

    return S_OK;
}

LRESULT CALLBACK WindowWin::WindowProcStatic(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (WindowWin* window = reinterpret_cast<WindowWin*>(GetWindowLongPtr(hWnd, GWLP_USERDATA)))
    {
        return window->WindowProc(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void WindowWin::InitOpenGLExtensions()
{
    WNDCLASSA window_class = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = DefWindowProcA,
        .hInstance = GetModuleHandle(nullptr),
        .lpszClassName = "Dummy_Window",
    };

    if (!RegisterClassA(&window_class))
        FL_CORE_ERROR("Failed to register dummy OpenGL window");

    HWND dummyWindow = CreateWindowExA(0, window_class.lpszClassName, "Dummy OpenGL Window", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0,
                                        0, window_class.hInstance, nullptr);
    if (!dummyWindow)
        FL_CORE_ERROR("Failed to create dummy OpenGL window.");

    HDC dummyDC = GetDC(dummyWindow);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    const int pixelFormat = ChoosePixelFormat(dummyDC, &pfd);
    if (!pixelFormat)
    {
        FL_CORE_ERROR("Failed to find a suitable pixel format.");
    }
    if (!SetPixelFormat(dummyDC, pixelFormat, &pfd))
    {
        FL_CORE_ERROR("Failed to set the pixel format.");
    }

    const HGLRC dummyContext = wglCreateContext(dummyDC);
    if (!dummyContext)
    {
        FL_CORE_ERROR("Failed to create a dummy OpenGL rendering context.");
    }

    if (!wglMakeCurrent(dummyDC, dummyContext))
    {
        FL_CORE_ERROR("Failed to activate dummy OpenGL rendering context.");
    }

    gladLoaderLoadWGL(dummyDC);

    wglMakeCurrent(dummyDC, 0);
    wglDeleteContext(dummyContext);
    ReleaseDC(dummyWindow, dummyDC);
    DestroyWindow(dummyWindow);
}

LRESULT WindowWin::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    m_MouseDir.x = 0;
    m_MouseDir.y = 0;

    switch (msg)
    {
    // Activate\Deactivate:
    case WM_CLOSE:
    {
        m_EventQueue->PushEvent(std::make_shared<EventVariant>(WindowCloseEvent()));
        break;
    }

    // ALT+TAB after WM_CAPTURECHANGED
    // WIN button afer WM_CAPTURECHANGED (if SetCapture() or ClipCursor() Called)
    // Clicks in non-application area like Desktop
    case WM_ACTIVATEAPP:
    {
        if (LOWORD(wParam))
            m_IsAppActive = true;
        else
            m_IsAppActive = false;
        break;
    }

    // If user use ALT+TAB or clicks on to the other application -> Triggers this event then WM_ACTIVATEAPP(FALSE)
    // If Calls ReleaseCapture() -> Triggers only this event
    // If clicks Border (Non-Client area) \ Header \ Minimize \ Maximize \ Close \ Child windowses\controls \ other windowses of the same process
    case WM_CAPTURECHANGED:
    {
        // Click on Window Border\Buttons
        // To let Windows handle button click we can't call SetCursor(hwnd) ot ClipCursor() for this tick
        if (lParam == 0 && m_IsFrameAction)
        {
            m_IsFrameAction = false;
            if (m_InteractionMode == EInteractionMode::GAMING)
                SetGamingMode();
            break;
        }
    }

    // If the cursor has moved from someone else's window or desktop to the client area of your window
    // With each WM_MOUSEMOVE, Windows first checks whether the cursor needs to be changed. If so, it also sends WM_SETCURSOR
    // When the active window changes If the focus has moved to your window(e.g.Alt + Tab, click on the title, Win + Tab) — Windows wants
    // to make sure that the cursor is displayed correctly, and calls WM_SETCURSOR
    // When a window changes mode (client / non-client area)
    // When you call SetCapture() or ReleaseCapture()
    // // Even if windows is not in focus!
    case WM_SETCURSOR:
    {
        if (!m_HasInputFocus)
            return DefWindowProc(hWnd, msg, wParam, lParam);

        if (m_InteractionMode == EInteractionMode::GAMING)
            SetCursor(nullptr);
        return TRUE;
        break;
    }


    case WM_MOUSEACTIVATE:
    {
        if (HIWORD(lParam) == WM_LBUTTONDOWN)
        {
            if (LOWORD(lParam) != HTCLIENT)
                m_IsFrameAction = true;
        }
        break;
    }

    case WM_PAINT:
    {
        if (!m_IsPainted || m_IsResizing || m_Props.mode == MINIMIZED)
            return 0;

        m_IsPainted = false;
        m_EventQueue->PushEvent(std::make_shared<EventVariant>(WindowValidateEvent()));
        return 0;
    }

    // Window Rsize:
    case WM_ENTERSIZEMOVE:
    {
        m_IsResizing = true;
        POINT point{};
        GetCursorPos(&point);
        ScreenToClient(hWnd, &point);
        m_EventQueue->PushEvent(std::make_shared<EventVariant>(WindowStartResizeEvent(m_XPos, m_YPos, m_CurrentWidth, m_CurrentHeigth, point.x, point.y)));
        break;
    }
    case WM_SIZE:
    {
        m_CurrentWidth = LOWORD(lParam);
        m_CurrentHeigth = HIWORD(lParam);

        RECT rect;
        GetWindowRect(hWnd, &rect);
        m_XPos = rect.left;
        m_YPos = rect.top;
        m_EventQueue->PushEvent(std::make_shared<EventVariant>(WindowResizeEvent(m_XPos, m_YPos, m_CurrentWidth, m_CurrentHeigth)));

        SetWindowMode(wParam);

        if (wParam == SIZE_RESTORED)
        {
            POINT cursor;
            GetCursorPos(&cursor);
            if (PtInRect(&rect, cursor))
            {
                // Cursor is still inside window area
            }
            else
            {
                // Cursor is outside window area -> unlock mouse -> killfocus
                UnlockMouse();
                SetForegroundWindow(GetDesktopWindow());
            }
        }
        break;
    }
    case WM_EXITSIZEMOVE:
    {
        m_IsResizing = false;

        m_EventQueue->PushEvent(std::make_shared<EventVariant>(WindowEndResizeEvent(m_XPos, m_YPos, m_CurrentWidth, m_CurrentHeigth)));
        break;
    }

    // Input:
    // Raw input:
    case WM_INPUT:
    {
        if (m_IsAppActive)
        {
            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            LPBYTE lpb = new BYTE[dwSize];

            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
                FL_CORE_ERROR("GetRawInputData does not return correct size !");

            RAWINPUT* raw = (RAWINPUT*)lpb;

            if (raw->header.dwType == RIM_TYPEKEYBOARD)
            {
                bool isDown = (raw->data.keyboard.Flags & RI_KEY_BREAK) == 0;
                EKeyCode crossplatformKeycode = GetKeyCode(raw->data.keyboard.VKey);
                if (isDown)
                {
                    m_PressedKeys[crossplatformKeycode] = Input::EKeyState::KEY_PRESSED;
                    m_EventQueue->PushEvent(std::make_shared<EventVariant>(KeyPressedEvent(crossplatformKeycode, 1)));
                }
                else
                {
                    m_PressedKeys[crossplatformKeycode] = Input::EKeyState::KEY_RELEASED;
                    m_EventQueue->PushEvent(std::make_shared<EventVariant>(KeyReleasedEvent(crossplatformKeycode)));
                }
            }
            else if (raw->header.dwType == RIM_TYPEMOUSE)
            {
                m_PrevCursorPos = m_CursorPos;
                m_MouseDir.x = raw->data.mouse.lLastX;
                m_MouseDir.y = raw->data.mouse.lLastY;
                m_CursorPos.x += m_MouseDir.x;
                m_CursorPos.y += m_MouseDir.y;
                m_BufferX += m_MouseDir.x;
                m_BufferY += m_MouseDir.y;

                USHORT buttonFlags = raw->data.mouse.usButtonFlags;
                if (buttonFlags != 0)
                {
                    EMouseCode button = Mouse::None;
                    if (buttonFlags & 0x001)
                    {
                        button = Mouse::Button0;
                        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(button)));
                    }
                    if (buttonFlags & 0x0002)
                    {
                        button = Mouse::Button0;
                        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(button)));
                    }
                    if (buttonFlags & 0x0004)
                    {
                        button = Mouse::Button1;
                        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(button)));
                    }
                    if (buttonFlags & 0x0008)
                    {
                        button = Mouse::Button1;
                        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(button)));
                    }
                    if (buttonFlags & 0x0010)
                    {
                        button = Mouse::Button2;
                        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(button)));
                    }
                    if (buttonFlags & 0x0020)
                    {
                        button = Mouse::Button2;
                        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(button)));
                    }
                    if (buttonFlags & 0x0400)  // Mouse Wheel vertical
                    {
                        SHORT wheelDelta = (SHORT)raw->data.mouse.usButtonData;
                        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseScrolledEvent(wheelDelta, 0.f)));
                    }
                    if (buttonFlags & 0x0800)  // Mouse Wheel Horizontal
                    {
                        SHORT wheelDelta = (SHORT)raw->data.mouse.usButtonData;
                        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseScrolledEvent(0.f, wheelDelta)));
                    }
                }
            };

            delete[] lpb;
        }
        break;
    }

    // Mouse Events
    // Even if windows is not in focus!
    case WM_MOUSEMOVE:
    {
        if (!m_HasInputFocus)
            return true;

        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseMovedEvent(m_MouseDir.x, m_MouseDir.y)));
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        EMouseCode button = Mouse::None;
        switch (msg)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            button = Mouse::Button0;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            button = Mouse::Button1;
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            button = Mouse::Button2;
            break;
        }

        if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN)
        {
            m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(button)));
        }
        else
        {
            m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(button)));
        }
        break;
    }
    case WM_MOUSEWHEEL:
    {
        float yOffset = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseScrolledEvent(yOffset, 0.f)));
        break;
    }
    case WM_MOUSEHWHEEL:
    {
        float xOffset = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
        m_EventQueue->PushEvent(std::make_shared<EventVariant>(MouseScrolledEvent(0.f, xOffset)));
        break;
    }

    // Keyboard events:

    // Keyboard focus:
    // The user clicks on your window → it becomes active and gets focus → WM_SETFOCUS.
    // The user Alt + Tab's to your window → WM_SETFOCUS.
    // SetFocus(hWnd) is called manually → WM_SETFOCUS.
    // After creating the window, if you immediately give it focus → it will also come.
    case WM_SETFOCUS:
    {
        m_HasInputFocus = true;
        if (m_IsFrameAction)
            break;

        if (m_InteractionMode == EInteractionMode::GAMING)
            SetGamingMode();

        break;
    }

    // You Alt+Tab'd to another application → your window gets WM_KILLFOCUS.
    // You clicked another window → the current window lost focus → WM_KILLFOCUS.
    // You called SetFocus() on another window → the old window gets WM_KILLFOCUS, the new one gets WM_SETFOCUS.
    case WM_KILLFOCUS:
    {
        m_IsFrameAction = m_HasInputFocus = false;
        UnlockMouse();
        break;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        bool isKeyDown = (msg == WM_KEYDOWN);
        int windowKeycode = static_cast<int>(wParam);
        EKeyCode crossplatformKeycode = GetKeyCode(windowKeycode);

        if (isKeyDown)
        {
            int repeatCount = (lParam >> 16) & 0xFF;
            bool firstPress = !(lParam & (1 << 30));
            m_PressedKeys[crossplatformKeycode] = firstPress ? Input::EKeyState::KEY_PRESSED : Input::EKeyState::KEY_REPEAT;
            m_EventQueue->PushEvent(std::make_shared<EventVariant>(KeyPressedEvent(crossplatformKeycode, repeatCount)));
        }
        else
        {
            m_PressedKeys[crossplatformKeycode] = Input::EKeyState::KEY_RELEASED;
            m_EventQueue->PushEvent(std::make_shared<EventVariant>(KeyReleasedEvent(crossplatformKeycode)));
        }
        break;
    }

    default:
    {
        break;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

WindowWin::WindowWin(const WindowProps& props, EventQueue& eventQueue)
    : m_EventQueue(static_cast<EventQueueWin*>(&eventQueue))
    , m_Hinstance(GetModuleHandle(nullptr))
    , m_Props(props)
    , m_LastMouse{Input::MOUSE_NONE, Mouse::None}
    , m_CursorPos{0.f, 0.f}
    , m_WinThread{}
    , m_WinThreadID(nullptr)
    , m_OnThreadCreated(CreateEvent(nullptr, FALSE, FALSE, nullptr))
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
    , m_PrevMouseDir(0.f, 0.f)
    , m_MouseDir(0.f, 0.f)
    , m_MouseWheelData(std::make_pair(0.f, 0.f))
{
    m_WinThread = CreateThread(nullptr, 0, WinThreadMain, this, 0, m_WinThreadID);
    WaitForSingleObject(m_OnThreadCreated, INFINITE);
}

void WindowWin::OnUpdate(float dlTime)
{
    glm::vec2 tmp = m_MouseDir;
    m_MouseDir.x = std::lerp(m_PrevMouseDir.x, m_BufferX, 0.5f);
    m_MouseDir.y = std::lerp(m_PrevMouseDir.y, m_BufferY, 0.5f);
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

void WindowWin::OnPostUpdate(float dlTime)
{
    // TODO
}

void WindowWin::OnFixedUpdate()
{
    // TODO
}

const void* WindowWin::GetNativeHandle() const
{
    return m_HWND;
}

Input::EKeyState WindowWin::GetKeyState(EKeyCode keyCode) const
{
    return m_PressedKeys[keyCode];
}

Input::EMouseState WindowWin::GetMouseState(EMouseCode mouseCode) const
{
    return m_LastMouse.MouseCode == mouseCode ? m_LastMouse.State : Input::EMouseState::MOUSE_NONE;
}

std::pair<float, float> WindowWin::GetMouseWheelScrollData() const
{
    return m_MouseWheelData;
}

void WindowWin::GetMousePos(OUT float& xPos, OUT float& yPos) const
{
    xPos = m_CursorPos.x;
    yPos = m_CursorPos.y;
}

std::unique_ptr<Window> Window::CreateAppWindow(const WindowProps& props, EventQueue& eventQueue)
{
    return std::make_unique<WindowWin>(props, eventQueue);
}

void WindowWin::SetMousePos(float x, float y)
{
    m_PrevCursorPos = m_CursorPos;
    m_CursorPos.x = x;
    m_CursorPos.y = y;
}

void WindowWin::SetMouseWheelScrollData(float x, float y)
{
    m_MouseWheelData.first = x;
    m_MouseWheelData.second = y;
}

void WindowWin::SetGamingMode()
{
    RECT rect;
    GetClientRect(m_HWND, &rect);
    POINT ul = {rect.left, rect.top};
    POINT lr = {rect.right, rect.bottom};
    MapWindowPoints(m_HWND, nullptr, &ul, 1);
    MapWindowPoints(m_HWND, nullptr, &lr, 1);

    RECT clipRect = {ul.x, ul.y, lr.x, lr.y};
    ClipCursor(&clipRect);

    POINT cursor{};
    GetCursorPos(&cursor);
    m_PrevCursorPos = m_CursorPos;
    m_CursorPos.x = cursor.x;
    m_CursorPos.y = cursor.y;

    SetCursor(NULL);
}

void WindowWin::UnlockMouse()
{
    if (m_InteractionMode == EInteractionMode::GAMING)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        ClipCursor(nullptr);
    }
}

void WindowWin::SetWindowMode(WPARAM mode)
{
    switch (mode)
    {
    case SIZE_MINIMIZED:
        m_Props.mode = MINIMIZED;
        break;
    case SIZE_MAXIMIZED:
        m_Props.mode = MAXIMIZED;
        break;
    case SIZE_RESTORED:
        m_Props.mode = RESTORED;
        break;
    }
}
}  // namespace Fleur
