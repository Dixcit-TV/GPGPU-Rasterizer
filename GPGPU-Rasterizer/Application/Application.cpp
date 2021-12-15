#include "pch.h"

#include <vector>
#include "Mesh/TriangleMesh.h"
#include "Material/Material.h"
#include "WindowAndViewport/Window.h"
#include "Renderer/HardwareRenderer.h"

int wmain(int argc, wchar_t* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    HardwareRenderer hwRenderer{};

    wchar_t windowName[]{ TEXT("GPU Rasterizer - Dixcit") };
    Window wnd{ windowName, 1920u, 1080u };
	wnd.Init();

	hwRenderer.Initialize(wnd);

	std::vector positions{
		DirectX::XMFLOAT3{0.f, 0.5f, 0.5f}
		, DirectX::XMFLOAT3{0.5f, -0.5f, 0.5f}
		, DirectX::XMFLOAT3{-0.5f, -0.5f, 0.5f}
	};

	std::vector<uint32_t> indices{ 0, 1, 2 };

	TriangleMesh mesh{ std::move(positions), {}, {}, std::move(indices) };
	Material mat{ hwRenderer.GetDevice(), L"./Resources/HardwareShader/VS_PosNormUV.hlsl", nullptr, nullptr, nullptr, L"Resources/HardwareShader/PS_LambertDiffuse.hlsl" };
	mesh.SetMaterial(hwRenderer.GetDevice(), &mat);

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

		hwRenderer.ClearBuffers();
		hwRenderer.DrawIndexed(&mesh);
		hwRenderer.Present();
	}
}