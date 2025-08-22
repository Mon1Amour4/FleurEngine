#include "WindowWin.h"

#include <glad/wgl.h>

#include "InputWin.h"
#include "KeyCodesWin.h"
#include "Log.h"

namespace Fleur
{

void WindowWin::SetTitle(std::string title)
{
    SetWindowText(_hwnd, std::string(_props.Title + " " + title).c_str());
}

DWORD WINAPI WindowWin::WinThreadMain(LPVOID lpParameter)
{
    InitOpenGLExtensions();

    WindowWin* window = static_cast<WindowWin*>(lpParameter);

    static TCHAR buffer[32] = TEXT("");
#ifdef UNICODE
    MultiByteToWideChar(CP_UTF8, 0, props.Title.c_str(), -1, buffer, _countof(buffer));
#else
    FL_CORE_ASSERT(sprintf_s(buffer, window->_props.Title.c_str()), "")
#endif
    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpszClassName = window->_props.APP_WINDOW_CLASS_NAME;
    wndClass.hInstance = window->_hinstance;
    wndClass.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.lpfnWndProc = WindowProcStatic;

    RegisterClassEx(&wndClass);

    DWORD style = WS_OVERLAPPEDWINDOW | WS_SYSMENU;
    RECT rect;
    rect.left = window->_props.x;
    rect.top = window->_props.y;
    rect.right = rect.left + window->_props.Width;
    rect.bottom = rect.top + window->_props.Height;

    AdjustWindowRect(&rect, style, true);

    window->_hwnd = CreateWindowEx(0, window->_props.APP_WINDOW_CLASS_NAME, buffer, style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                                   nullptr, nullptr, window->_hinstance, nullptr);

    FL_CORE_ASSERT(window->_hwnd, "[AppWindow] hasn't been initialized!");

    // Associate this instance with the HWND
    SetWindowLongPtr(window->_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

    ShowWindow(window->_hwnd, SW_SHOW);
    SetEvent(window->_onThreadCreated);

    // Generic Mouse
    window->Rid[0].usUsagePage = 0x01;
    window->Rid[0].usUsage = 0x02;
    window->Rid[0].dwFlags = RIDEV_INPUTSINK;
    window->Rid[0].hwndTarget = window->_hwnd;

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

    DestroyWindow(window->_hwnd);
    DestroyIcon(wndClass.hIcon);
    DestroyCursor(wndClass.hCursor);
    UnregisterClass(window->_props.APP_WINDOW_CLASS_NAME, window->_hinstance);

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

    HWND dummy_window = CreateWindowExA(0, window_class.lpszClassName, "Dummy OpenGL Window", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0,
                                        0, window_class.hInstance, nullptr);
    if (!dummy_window)
        FL_CORE_ERROR("Failed to create dummy OpenGL window.");

    HDC dummy_dc = GetDC(dummy_window);

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

    const int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
    if (!pixel_format)
    {
        FL_CORE_ERROR("Failed to find a suitable pixel format.");
    }
    if (!SetPixelFormat(dummy_dc, pixel_format, &pfd))
    {
        FL_CORE_ERROR("Failed to set the pixel format.");
    }

    const HGLRC dummy_context = wglCreateContext(dummy_dc);
    if (!dummy_context)
    {
        FL_CORE_ERROR("Failed to create a dummy OpenGL rendering context.");
    }

    if (!wglMakeCurrent(dummy_dc, dummy_context))
    {
        FL_CORE_ERROR("Failed to activate dummy OpenGL rendering context.");
    }

    gladLoaderLoadWGL(dummy_dc);

    wglMakeCurrent(dummy_dc, 0);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, dummy_dc);
    DestroyWindow(dummy_window);
}

LRESULT WindowWin::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    _mouseDir.x = 0;
    _mouseDir.y = 0;

    switch (msg)
    {
    // Activate\Deactivate:
    case WM_CLOSE:
    {
        FL_CORE_INFO("WM_CLOSE");
        _eventQueue->PushEvent(std::make_shared<EventVariant>(WindowCloseEvent()));
        break;
    }

    // ALT+TAB after WM_CAPTURECHANGED
    // WIN button afer WM_CAPTURECHANGED (if SetCapture() or ClipCursor() Called)
    // Clicks in non-application area like Desktop
    case WM_ACTIVATEAPP:
    {
        if (LOWORD(wparam))
            appActive = true;
        else
            appActive = false;
        break;
    }

    // If user use ALT+TAB or clicks on to the other application -> Triggers this event then WM_ACTIVATEAPP(FALSE)
    // If Calls ReleaseCapture() -> Triggers only this event
    // If clicks Border (Non-Client area) \ Header \ Minimize \ Maximize \ Close \ Child windowses\controls \ other windowses of the same process
    case WM_CAPTURECHANGED:
    {
        // Click on Window Border\Buttons
        // To let Windows handle button click we can't call SetCursor(hwnd) ot ClipCursor() for this tick
        if (lparam == 0 && frame_action)
        {
            frame_action = false;
            if (interaction_mode == InteractionMode::GAMING)
                set_gaming_mode();
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
        if (!has_input_focus)
            return DefWindowProc(hwnd, msg, wparam, lparam);

        if (interaction_mode == InteractionMode::GAMING)
            SetCursor(nullptr);
        return TRUE;
        break;
    }


    case WM_MOUSEACTIVATE:
    {
        if (HIWORD(lparam) == WM_LBUTTONDOWN)
        {
            if (LOWORD(lparam) != HTCLIENT)
                frame_action = true;
        }
        break;
    }

    case WM_PAINT:
    {
        if (!isPainted || isResizing || _props.mode == MINIMIZED)
            return 0;

        isPainted = false;
        _eventQueue->PushEvent(std::make_shared<EventVariant>(WindowValidateEvent()));
        return 0;
    }

    // Window Rsize:
    case WM_ENTERSIZEMOVE:
    {
        isResizing = true;
        POINT Point{};
        GetCursorPos(&Point);
        ScreenToClient(hwnd, &Point);
        _eventQueue->PushEvent(std::make_shared<EventVariant>(WindowStartResizeEvent(_xPos, _yPos, _currentWidth, _currentHeigth, Point.x, Point.y)));
        break;
    }
    case WM_SIZE:
    {
        _currentWidth = LOWORD(lparam);
        _currentHeigth = HIWORD(lparam);

        RECT Rect;
        GetWindowRect(hwnd, &Rect);
        _xPos = Rect.left;
        _yPos = Rect.top;
        _eventQueue->PushEvent(std::make_shared<EventVariant>(WindowResizeEvent(_xPos, _yPos, _currentWidth, _currentHeigth)));

        SetWindowMode(wparam);

        if (wparam == SIZE_RESTORED)
        {
            POINT Cursor;
            GetCursorPos(&Cursor);
            if (PtInRect(&Rect, Cursor))
            {
                // Cursor is still inside window area
            }
            else
            {
                // Cursor is outside window area -> unlock mouse -> killfocus
                unlock_mouse();
                SetForegroundWindow(GetDesktopWindow());
            }
        }
        break;
    }
    case WM_EXITSIZEMOVE:
    {
        isResizing = false;

        _eventQueue->PushEvent(std::make_shared<EventVariant>(WindowEndResizeEvent(_xPos, _yPos, _currentWidth, _currentHeigth)));
        break;
    }

    // Input:
    // Raw input:
    case WM_INPUT:
    {
        if (appActive)
        {
            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            LPBYTE lpb = new BYTE[dwSize];

            if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
                FL_CORE_ERROR("GetRawInputData does not return correct size !");

            RAWINPUT* raw = (RAWINPUT*)lpb;

            if (raw->header.dwType == RIM_TYPEKEYBOARD)
            {
                bool isDown = (raw->data.keyboard.Flags & RI_KEY_BREAK) == 0;
                KeyCode crossplatform_keycode = GetKeyCode(raw->data.keyboard.VKey);
                if (isDown)
                {
                    pressed_keys[crossplatform_keycode] = Input::KeyState::KEY_PRESSED;
                    _eventQueue->PushEvent(std::make_shared<EventVariant>(KeyPressedEvent(crossplatform_keycode, 1)));
                }
                else
                {
                    pressed_keys[crossplatform_keycode] = Input::KeyState::KEY_RELEASED;
                    _eventQueue->PushEvent(std::make_shared<EventVariant>(KeyReleasedEvent(crossplatform_keycode)));
                }
            }
            else if (raw->header.dwType == RIM_TYPEMOUSE)
            {
                _prevCursorPos = _cursorPos;
                _mouseDir.x = raw->data.mouse.lLastX;
                _mouseDir.y = raw->data.mouse.lLastY;
                _cursorPos.x += _mouseDir.x;
                _cursorPos.y += _mouseDir.y;
                // FL_CORE_INFO("RIM_TYPEMOUSE");
                USHORT button_flags = raw->data.mouse.usButtonFlags;
                if (button_flags != 0)
                {
                    MouseCode button = Mouse::None;
                    if (button_flags & 0x001)
                    {
                        button = Mouse::Button0;
                        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(button)));
                    }
                    if (button_flags & 0x0002)
                    {
                        button = Mouse::Button0;
                        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(button)));
                    }
                    if (button_flags & 0x0004)
                    {
                        button = Mouse::Button1;
                        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(button)));
                    }
                    if (button_flags & 0x0008)
                    {
                        button = Mouse::Button1;
                        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(button)));
                    }
                    if (button_flags & 0x0010)
                    {
                        button = Mouse::Button2;
                        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(button)));
                    }
                    if (button_flags & 0x0020)
                    {
                        button = Mouse::Button2;
                        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(button)));
                    }
                    if (button_flags & 0x0400)  // Mouse Wheel vertical
                    {
                        SHORT wheelDelta = (SHORT)raw->data.mouse.usButtonData;
                        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseScrolledEvent(wheelDelta, 0.f)));
                    }
                    if (button_flags & 0x0800)  // Mouse Wheel Horizontal
                    {
                        SHORT wheelDelta = (SHORT)raw->data.mouse.usButtonData;
                        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseScrolledEvent(0.f, wheelDelta)));
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
        if (!has_input_focus)
            return true;

        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseMovedEvent(_mouseDir.x, _mouseDir.y)));
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        MouseCode button = Mouse::None;
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
            _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(button)));
        }
        else
        {
            _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(button)));
        }
        break;
    }
    case WM_MOUSEWHEEL:
    {
        float yOffset = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam));
        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseScrolledEvent(yOffset, 0.f)));
        break;
    }
    case WM_MOUSEHWHEEL:
    {
        float xOffset = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam));
        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseScrolledEvent(0.f, xOffset)));
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
        has_input_focus = true;
        if (frame_action)
            break;

        if (interaction_mode == InteractionMode::GAMING)
            set_gaming_mode();

        break;
    }

    // You Alt+Tab'd to another application → your window gets WM_KILLFOCUS.
    // You clicked another window → the current window lost focus → WM_KILLFOCUS.
    // You called SetFocus() on another window → the old window gets WM_KILLFOCUS, the new one gets WM_SETFOCUS.
    case WM_KILLFOCUS:
    {
        frame_action = has_input_focus = false;
        unlock_mouse();
        break;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        bool isKeyDown = (msg == WM_KEYDOWN);
        int window_keycode = static_cast<int>(wparam);
        KeyCode crossplatform_keycode = GetKeyCode(window_keycode);

        if (isKeyDown)
        {
            int repeatCount = (lparam >> 16) & 0xFF;
            bool firstPress = !(lparam & (1 << 30));
            pressed_keys[crossplatform_keycode] = firstPress ? Input::KeyState::KEY_PRESSED : Input::KeyState::KEY_REPEAT;
            _eventQueue->PushEvent(std::make_shared<EventVariant>(KeyPressedEvent(crossplatform_keycode, repeatCount)));
        }
        else
        {
            pressed_keys[crossplatform_keycode] = Input::KeyState::KEY_RELEASED;
            _eventQueue->PushEvent(std::make_shared<EventVariant>(KeyReleasedEvent(crossplatform_keycode)));
        }
        break;
    }

    default:
    {
        break;
    }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

WindowWin::WindowWin(const WindowProps& props, EventQueue& eventQueue)
    : _eventQueue(dynamic_cast<EventQueueWin*>(&eventQueue))
    , _hinstance(GetModuleHandle(nullptr))
    , _props(props)
    , _lastMouse{Input::MOUSE_NONE, Mouse::None}
    , _cursorPos{0.f, 0.f}
    , _winThread{}
    , _winThreadID(nullptr)
    , _onThreadCreated(CreateEvent(nullptr, FALSE, FALSE, nullptr))
    , isResizing(false)
    , isPainted(true)
    , frame_action(false)
    , _currentWidth(props.Width)
    , _currentHeigth(props.Height)
    , _xPos(props.x)
    , _yPos(props.y)
    , _prevCursorPos(_cursorPos)
    , pressed_keys{Input::KeyState::KEY_NONE}
    , interaction_mode(InteractionMode::GAMING)
    , is_first_launch(true)
    , has_input_focus(false)
    , appActive(false)
    , _mouseDir(0.f, 0.f)
    , mouse_wheel_data(std::make_pair(0.f, 0.f))
{
    _winThread = CreateThread(nullptr, 0, WinThreadMain, this, 0, _winThreadID);
    WaitForSingleObject(_onThreadCreated, INFINITE);
}

void WindowWin::OnUpdate(float dlTime)
{
    if (!has_input_focus)
        if (isResizing || _props.mode == MINIMIZED)
        {
            FL_CORE_INFO("stop rendering");
            return;
        }
    _eventQueue->PushEvent(std::make_shared<EventVariant>(AppRenderEvent()));
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
    return _hwnd;
}

Input::KeyState WindowWin::GetKeyState(KeyCode keyCode) const
{
    return pressed_keys[keyCode];
}

Input::MouseState WindowWin::GetMouseState(MouseCode mouseCode) const
{
    return _lastMouse.mouseCode == mouseCode ? _lastMouse.state : Input::MouseState::MOUSE_NONE;
}

std::pair<float, float> WindowWin::GetMouseWheelScrollData() const
{
    return mouse_wheel_data;
}

void WindowWin::GetMousePos(OUT float& xPos, OUT float& yPos) const
{
    xPos = _cursorPos.x;
    yPos = _cursorPos.y;
}

std::unique_ptr<Window> Window::CreateAppWindow(const WindowProps& props, EventQueue& eventQueue)
{
    return std::make_unique<WindowWin>(props, eventQueue);
}

void WindowWin::SetMousePos(float x, float y)
{
    _prevCursorPos = _cursorPos;
    _cursorPos.x = x;
    _cursorPos.y = y;
}

void WindowWin::SetMouseWheelScrollData(float x, float y)
{
    mouse_wheel_data.first = x;
    mouse_wheel_data.second = y;
}

void WindowWin::set_gaming_mode()
{
    RECT rect;
    GetClientRect(_hwnd, &rect);
    POINT ul = {rect.left, rect.top};
    POINT lr = {rect.right, rect.bottom};
    MapWindowPoints(_hwnd, nullptr, &ul, 1);
    MapWindowPoints(_hwnd, nullptr, &lr, 1);

    RECT clipRect = {ul.x, ul.y, lr.x, lr.y};
    ClipCursor(&clipRect);

    POINT Cursor{};
    GetCursorPos(&Cursor);
    _prevCursorPos = _cursorPos;
    _cursorPos.x = Cursor.x;
    _cursorPos.y = Cursor.y;

    SetCursor(NULL);
}

void WindowWin::unlock_mouse()
{
    if (interaction_mode == InteractionMode::GAMING)
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
        _props.mode = MINIMIZED;
        break;
    case SIZE_MAXIMIZED:
        _props.mode = MAXIMIZED;
        break;
    case SIZE_RESTORED:
        _props.mode = RESTORED;
        break;
    }
}
}  // namespace Fleur
