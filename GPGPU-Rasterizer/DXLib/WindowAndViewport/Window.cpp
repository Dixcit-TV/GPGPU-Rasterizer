#include "pch.h"
#include "Window.h"

constexpr wchar_t wClassName[] = L"AppWindowClass";

Window::Window(const wchar_t* name, uint16_t width, uint16_t height)
    : m_Name{ name }
    , m_Width{ width }
    , m_Height{ height }
    , m_Handle{ nullptr }
    , m_WndProcImpl{ nullptr }
    , m_bIsInitialized{ false }
{}

void Window::Init(WNDPROC wndProcExt)
{
    if (m_bIsInitialized)
        return;

    m_WndProcImpl = wndProcExt;
    // Create the window.

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = wClassName;
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        //HRESULT res{ HRESULT_FROM_WIN32(GetLastError()) };
        return;
    }

    m_Handle = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        wClassName,
        m_Name.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_Width,
        m_Height,
        nullptr,
        nullptr,
        wc.hInstance,
        this
    );

    if (!m_Handle)
    {
        //HRESULT res{ HRESULT_FROM_WIN32(GetLastError()) };
        return;
    }

    ShowWindow(m_Handle, SW_SHOW);

    m_bIsInitialized = true;
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
            {
                CREATESTRUCT* pCS = reinterpret_cast<CREATESTRUCT*>(lParam);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCS->lpCreateParams));
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            {
                Window* pThisWindow = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                if (pThisWindow && pThisWindow->m_WndProcImpl)
                    return pThisWindow->m_WndProcImpl(hwnd, msg, wParam, lParam);
            }   
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}