#include "pch.h"
#include "WindowAndViewport/Window.h"
#include "Renderer/HardwareRenderer.h"

int wmain(int argc, wchar_t* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    HardwareRenderer a{};

    wchar_t windowName[]{ TEXT("GPU Rasterizer - Dixcit") };
    Window wnd{ windowName, 1920u, 1080u };
    wnd.Init();

    a.Initialize(wnd);

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while (msg.message != WM_QUIT)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				break;
		}


	}
}