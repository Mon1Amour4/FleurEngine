#include "WindowWin.h"

#include "InputWin.h"
#include "Log.h"

#define LAST_CODE UINT16_MAX

namespace Fuego
{

DWORD WINAPI WindowWin::WinThreadMain(LPVOID lpParameter)
{
    WindowWin* _wnd = static_cast<WindowWin*>(lpParameter);

    static TCHAR buffer[32] = TEXT("");
#ifdef UNICODE
    MultiByteToWideChar(CP_UTF8, 0, props.Title.c_str(), -1, buffer, _countof(buffer));
#else
    sprintf_s(buffer, _wnd->_props.Title.c_str());
#endif
    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpszClassName = _wnd->_props.APP_WINDOW_CLASS_NAME;
    wndClass.hInstance = _wnd->_hinstance;
    wndClass.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.lpfnWndProc = WindowProcStatic;

    RegisterClassEx(&wndClass);

    DWORD style = WS_OVERLAPPEDWINDOW | WS_SYSMENU;
    RECT rect;
    rect.left = _wnd->_props.x;
    rect.top = _wnd->_props.y;
    rect.right = rect.left + _wnd->_props.Width;
    rect.bottom = rect.top + _wnd->_props.Height;

    AdjustWindowRect(&rect, style, true);

    _wnd->_hwnd = CreateWindowEx(0, _wnd->_props.APP_WINDOW_CLASS_NAME, buffer, style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                                 nullptr, nullptr, _wnd->_hinstance, nullptr);

    FU_CORE_ASSERT(_wnd->_hwnd, "[AppWindow] hasn't been initialized!");

    // Associate this instance with the HWND
    SetWindowLongPtr(_wnd->_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(_wnd));

    ShowWindow(_wnd->_hwnd, SW_SHOW);

    FU_CORE_ASSERT(Input::Init(new InputWin()), "[Input] hasn't been initialized!");
    SetEvent(_wnd->_onThreadCreated);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0u, 0u))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(_wnd->_hwnd);
    DestroyIcon(wndClass.hIcon);
    DestroyCursor(wndClass.hCursor);
    UnregisterClass(_wnd->_props.APP_WINDOW_CLASS_NAME, _wnd->_hinstance);

    return S_OK;
}

LRESULT CALLBACK WindowWin::WindowProcStatic(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WindowWin* window = reinterpret_cast<WindowWin*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (window)
    {
        return window->WindowProc(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT WindowWin::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_PAINT:
        _eventQueue->PushEvent(std::make_shared<EventVariant>(AppRenderEvent()));
        return 0;
    case WM_ACTIVATE:
        break;

    case WM_MOUSEMOVE:
    {
        float x = static_cast<float>(GET_X_LPARAM(lparam));
        float y = static_cast<float>(GET_Y_LPARAM(lparam));
        _cursorPos = {x, y};
        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseMovedEvent(x, y)));
        break;
    }

    case WM_LBUTTONDOWN:
        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(Mouse::Button0)));
        break;

    case WM_LBUTTONUP:
        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(Mouse::Button0)));
        break;

    case WM_RBUTTONDOWN:
        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(Mouse::Button1)));
        break;

    case WM_RBUTTONUP:
        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(Mouse::Button1)));
        break;

    case WM_MBUTTONDOWN:
        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonPressedEvent(Mouse::Button2)));
        break;

    case WM_MBUTTONUP:
        _eventQueue->PushEvent(std::make_shared<EventVariant>(MouseButtonReleasedEvent(Mouse::Button2)));
        break;

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

    case WM_KEYDOWN:
    {
        int repeatCount = (lparam >> 16) & 0xFF;
        int keyCode = static_cast<int>(wparam);
        _eventQueue->PushEvent(std::make_shared<EventVariant>(KeyPressedEvent(keyCode, repeatCount)));
        _lastKey = {Input::KEY_PRESSED, (uint16_t)keyCode};
        break;
    }
    case WM_KEYUP:
        _eventQueue->PushEvent(std::make_shared<EventVariant>(KeyReleasedEvent(static_cast<int>(wparam))));
        break;

    case WM_CLOSE:
        _eventQueue->PushEvent(std::make_shared<EventVariant>(WindowCloseEvent()));
        break;

    case WM_SIZE:
    {
        UINT width = LOWORD(lparam);
        UINT height = HIWORD(lparam);

        _eventQueue->PushEvent(std::make_shared<EventVariant>(WindowResizeEvent(width, height)));
        break;
    }

    default:
    {
        _lastKey = {Input::KEY_NONE, LAST_CODE};
        _lastMouse = {Input::MOUSE_NONE, LAST_CODE};
        break;
    }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

WindowWin::WindowWin(const WindowProps& props, EventQueue& eventQueue)
    : _eventQueue(dynamic_cast<EventQueueWin*>(&eventQueue))
    , _hinstance(GetModuleHandle(nullptr))
    , _props(props)
    , _lastKey{Input::KEY_NONE, LAST_CODE}
    , _lastMouse{Input::MOUSE_NONE, LAST_CODE}
    , _cursorPos{0.f, 0.f}
    , _winThread{}
    , _winThreadID(nullptr)
    , _onThreadCreated(CreateEvent(nullptr, FALSE, FALSE, nullptr))
{
    _winThread = CreateThread(nullptr, 0, WinThreadMain, this, 0, _winThreadID);
    WaitForSingleObject(_onThreadCreated, INFINITE);
}

void WindowWin::Update()
{
}

void WindowWin::SetVSync(bool enabled)
{
    UNUSED(enabled);
}

bool WindowWin::IsVSync() const
{
    return true;
}

const void* WindowWin::GetNativeHandle() const
{
    return _hwnd;
}

Input::KeyState WindowWin::GetKeyState(KeyCode keyCode) const
{
    return _lastKey.keyCode == keyCode ? _lastKey.state : Input::KeyState::KEY_NONE;
}

Input::MouseState WindowWin::GetMouseState(MouseCode mouseCode) const
{
    return _lastMouse.mouseCode == mouseCode ? _lastMouse.state : Input::MouseState::MOUSE_NONE;
}

void WindowWin::GetCursorPos(OUT float& xPos, OUT float& yPos) const
{
    xPos = _cursorPos.x;
    yPos = _cursorPos.y;
}

std::unique_ptr<Window> Window::CreateAppWindow(const WindowProps& props, EventQueue& eventQueue)
{
    return std::make_unique<WindowWin>(props, eventQueue);
}
}  // namespace Fuego
