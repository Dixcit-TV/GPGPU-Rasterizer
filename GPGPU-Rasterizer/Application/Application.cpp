#include "pch.h"

#include <vector>
#include "Camera/Camera.h"
#include "Common/ObjReader.h"
#include "Common/Structs.h"
#include "Mesh/TriangleMesh.h"
#include "Material/Material.h"
#include "WindowAndViewport/Window.h"
#include "Renderer/HardwareRenderer.h"

int wmain(int argc, wchar_t* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	wchar_t windowName[]{ TEXT("GPU Rasterizer - Dixcit") };
	Window wnd{ windowName, 1920u, 1080u };
	HardwareRenderer hwRenderer{};
	Camera camera{ DirectX::XMFLOAT3{0.f, 0.f, -100.f}, DirectX::XMFLOAT3{0.f, 0.f, 1.f}, static_cast<float>(wnd.GetWidth()) / static_cast<float>(wnd.GetHeight()) };

	wnd.Init();

	hwRenderer.Initialize(wnd);

	std::vector<DirectX::XMFLOAT3> positions{};
	std::vector<DirectX::XMFLOAT3> normals{};
	std::vector<DirectX::XMFLOAT2> uvs{};
	std::vector<uint32_t> indices;

	//std::vector positions{
	//DirectX::XMFLOAT3{0.f, 5.f, 0.f}
	//, DirectX::XMFLOAT3{5.f, -5.f, 0.f}
	//, DirectX::XMFLOAT3{-5.f, -5.f, 0.f}
	//};

	//std::vector<uint32_t> indices{ 0, 1, 2 };

	ObjReader::LoadModel(L"./Resources/Models/vehicle.obj", positions, normals, uvs, indices);

	TriangleMesh mesh{ std::move(positions), std::move(normals), std::move(uvs), std::move(indices) };
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

		camera.Update();
		hwRenderer.ClearBuffers();
		hwRenderer.DrawIndexed(&camera, &mesh);
		hwRenderer.Present();
	}
}