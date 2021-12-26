#include "pch.h"
#include <vector>
#include "Camera/Camera.h"
#include "Common/ObjReader.h"
#include "Managers/TimeSettings.h"
#include "Mesh/TriangleMesh.h"
#include "Material/Material.h"
#include "Mesh/Mesh.h"
#include "Renderer/CompuRenderer.h"
#include "WindowAndViewport/Window.h"
#include "Renderer/HardwareRenderer.h"
#include "Renderer/Pipeline/NaivePipeline/NaiveMaterial.h"

//#define HARDWARE_RENDER
#define CUSTOM_RENDER

void mainDXRaster(const Window& window, Camera& camera);
void mainCompuRaster(const Window& window, Camera& camera);

LRESULT WndProc_Implementation(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int wmain(int argc, wchar_t* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	wchar_t windowName[]{ TEXT("GPU Rasterizer - Dixcit") };
	Window wnd{ windowName, 1920u, 1080u };
	wnd.Init(&WndProc_Implementation);
	Camera camera{ DirectX::XMFLOAT3{0.f, 0.f, -100.f}, DirectX::XMFLOAT3{0.f, 0.f, 1.f}, static_cast<float>(wnd.GetWidth()) / static_cast<float>(wnd.GetHeight()) };

#if defined(HARDWARE_RENDER)
	mainDXRaster(wnd, camera);
#elif defined(CUSTOM_RENDER)
	mainCompuRaster(wnd, camera);
#endif
}

void mainDXRaster(const Window& window, Camera& camera)
{
	TimeSettings& timeSettings = TimeSettings::GetInstance();

	HardwareRenderer hwRenderer{};
	hwRenderer.Initialize(window);

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

		timeSettings.Update();

		camera.Update(timeSettings.GetElapsed());
		hwRenderer.ClearBuffers();
		hwRenderer.DrawIndexed(&camera, &mesh);
		hwRenderer.Present();

		timeSettings.TrySleep();
	}
}

void mainCompuRaster(const Window& window, Camera& camera)
{
	TimeSettings& timeSettings = TimeSettings::GetInstance();

	CompuRaster::CompuRenderer dcRenderer{};
	dcRenderer.Initialize(window);

	std::vector<DirectX::XMFLOAT3> positions{};
	std::vector<DirectX::XMFLOAT3> normals{};
	std::vector<DirectX::XMFLOAT2> uvs{};
	std::vector<uint32_t> indices;

	//std::vector positions{
	//DirectX::XMFLOAT3{0.f, 4.f, 0.f}
	//, DirectX::XMFLOAT3{4.f, -4.f, 0.f}
	//, DirectX::XMFLOAT3{-4.f, -4.f, 0.f}
	//, DirectX::XMFLOAT3{6.f, 4.f, 2.f}
	//};

	//std::vector<uint32_t> indices{ 0, 1, 2, 0, 3, 1 };

	ObjReader::LoadModel(L"./Resources/Models/vehicle.obj", positions, normals, uvs, indices);

	CompuRaster::Mesh mesh{ std::move(positions), std::move(normals), std::move(uvs), std::move(indices) };
	CompuRaster::NaiveMaterial mat{ dcRenderer.GetDevice(), L"./Resources/SoftwareShader/TestPipeline.hlsl" };
	mesh.SetMaterial(dcRenderer.GetDevice(), &mat);

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

		timeSettings.Update();

		camera.Update(timeSettings.GetElapsed());
		dcRenderer.ClearBuffers();
		dcRenderer.Draw(&camera, &mesh);
		dcRenderer.Present();

		timeSettings.TrySleep();
	}
}

LRESULT WndProc_Implementation(HWND, UINT msg, WPARAM wParam, LPARAM)
{
	switch (msg)
	{
	case WM_KEYUP:
		{
			//NextScene
			if (wParam == VK_F1)
			{
				std::wcout << L"FPS: " << TimeSettings::GetInstance().GetFPS() << "\n";
				return 0;
			}
		}
		break;
	default: break;
	}

	return -1;
}