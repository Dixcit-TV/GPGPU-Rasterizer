#include "pch.h"
#include <vector>
#include "Camera/Camera.h"
#include "Common/ObjReader.h"
#include "Managers/TimeSettings.h"
#include "Mesh/TriangleMesh.h"
#include "Material/Material.h"
#include "Mesh/CompuMesh.h"
#include "Mesh/Mesh.h"
#include "Renderer/CompuRenderer.h"
#include "WindowAndViewport/Window.h"
#include "Renderer/HardwareRenderer.h"
#include "Renderer/Pipeline/NaivePipeline/NaiveMaterial.h"
#include "Renderer/Pipeline/Material.h"
#include "Renderer/Pipeline/Pipeline.h"

//#define HARDWARE_RENDER
#define CUSTOM_RENDER

//#define CUSTOM_RENDER_NAIVE
#define CUSTOM_RENDER_PIPELINE_BINNING

#define VEHICLE_OBJ
//#define BUNNY_OBJ
//#define FAIRYFOREST_OBJ

void mainDXRaster(const Window& window, Camera& camera, std::wstring meshPath);
void mainCompuRaster(const Window& window, Camera& camera, std::wstring meshPath);

LRESULT WndProc_Implementation(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int wmain(int argc, wchar_t* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	wchar_t windowName[]{ TEXT("GPU Rasterizer - Dixcit") };
	Window wnd{ windowName, 1280u, 720u };
	wnd.Init(&WndProc_Implementation);

#if defined(VEHICLE_OBJ)
	Camera camera{ DirectX::XMFLOAT3{0.f, 0.f, -37.f}, DirectX::XMFLOAT3{0.f, 0.f, 1.f}, static_cast<float>(wnd.GetWidth()) / static_cast<float>(wnd.GetHeight()) };
	std::wstring meshPath{ L"./Resources/Models/vehicle.obj" };
#elif defined(BUNNY_OBJ)
	Camera camera{ DirectX::XMFLOAT3{0.f, 1.2f, -5.f}, DirectX::XMFLOAT3{0.f, 0.f, 1.f}, static_cast<float>(wnd.GetWidth()) / static_cast<float>(wnd.GetHeight()) };
	std::wstring meshPath{ L"./Resources/Models/bunny.obj" };
#elif defined(FAIRYFOREST_OBJ)
	Camera camera{ DirectX::XMFLOAT3{0.f, 1.f, -5.f}, DirectX::XMFLOAT3{0.f, 0.f, 1.f}, static_cast<float>(wnd.GetWidth()) / static_cast<float>(wnd.GetHeight()) };
	std::wstring meshPath{ L"./Resources/Models/fairyforest.obj" };
#endif


#if defined(HARDWARE_RENDER)
	mainDXRaster(wnd, camera, meshPath);
#elif defined(CUSTOM_RENDER)
	mainCompuRaster(wnd, camera, meshPath);
#endif
}

void mainDXRaster(const Window& window, Camera& camera, std::wstring meshPath)
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

	ObjReader::LoadModel(meshPath, positions, normals, uvs, indices);

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

void mainCompuRaster(const Window& window, Camera& camera, std::wstring meshPath)
{
	TimeSettings& timeSettings = TimeSettings::GetInstance();

	CompuRaster::CompuRenderer dcRenderer{};
	dcRenderer.Initialize(window);

	std::vector<DirectX::XMFLOAT3> positions{};
	std::vector<DirectX::XMFLOAT3> normals{};
	std::vector<DirectX::XMFLOAT2> uvs{};
	std::vector<uint32_t> indices;

	//std::vector positions{
	//	DirectX::XMFLOAT3{0.f, 10.f, 0.f}
	//	, DirectX::XMFLOAT3{10.f, -10.f, 0.f}
	//	, DirectX::XMFLOAT3{-10.f, -10.f, 0.f}
	//	, DirectX::XMFLOAT3{14.f, 10.f, 5.f}
	//	, DirectX::XMFLOAT3{-20.f, 20.f, -5.f}
	//	, DirectX::XMFLOAT3{0.f, 0.f, -5.f}

	//};

	//std::vector normals{
	//DirectX::XMFLOAT3{0.f, 0.f, -1.f}
	//, DirectX::XMFLOAT3{0.f, 0.f, -1.f}
	//, DirectX::XMFLOAT3{0.f, 0.f, -1.f}
	//, DirectX::XMFLOAT3{0.f, 0.f, -1.f}
	//, DirectX::XMFLOAT3{0.f, 0.f, -1.f}
	//, DirectX::XMFLOAT3{0.f, 0.f, -1.f}
	//};

	//std::vector<uint32_t> indices{ 0, 1, 2, 0, 3, 1, 4, 0, 2, 4, 3, 0, 2, 5, 1 };

	ObjReader::LoadModel(meshPath, positions, normals, uvs, indices);
#if defined(CUSTOM_RENDER_NAIVE)
	CompuRaster::Mesh mesh{ std::move(positions), std::move(normals), std::move(uvs), std::move(indices) };
	CompuRaster::NaiveMaterial mat{ dcRenderer.GetDevice(), L"./Resources/SoftwareShader/TestPipeline.hlsl" };
	mesh.SetMaterial(dcRenderer.GetDevice(), &mat);

#elif defined(CUSTOM_RENDER_PIPELINE_BINNING)
	CompuRaster::CompuMesh mesh{ std::move(positions), std::move(normals), std::move(uvs), std::move(indices) };
	CompuRaster::Material mat{};
	mat.Init(dcRenderer.GetDevice(), L"./Resources/SoftwareShader/Pipeline/VertexShader.hlsl", L"");
	mesh.SetMaterial(dcRenderer.GetDevice(), &mat);

	CompuRaster::Pipeline pipeline{};
	//pipeline.Init(dcRenderer.GetDevice(), mesh.GetVertexCount(), static_cast<UINT>(std::size(indices) / 3), L"./Resources/SoftwareShader/Pipeline/GeometrySetup.hlsl", L"./Resources/SoftwareShader/Pipeline/Rasterizer.hlsl", L"./Resources/SoftwareShader/Pipeline/Rasterizer2.hlsl");
	//pipeline.Init(dcRenderer.GetDevice(), mesh.GetVertexCount(), static_cast<UINT>(std::size(indices) / 3), L"./Resources/SoftwareShader/Pipeline/GeometrySetup.hlsl", L"./Resources/SoftwareShader/Pipeline/Rasterizer.hlsl", L"./Resources/SoftwareShader/Pipeline/FineRasterizer2.hlsl");
	pipeline.Init(dcRenderer.GetDevice(), mesh.GetVertexCount(), static_cast<UINT>(std::size(indices) / 3), L"./Resources/SoftwareShader/Pipeline/GeometrySetup.hlsl", L"./Resources/SoftwareShader/Pipeline/BinRasterizer.hlsl", L"./Resources/SoftwareShader/Pipeline/TileRasterizer.hlsl", L"./Resources/SoftwareShader/Pipeline/FineRasterizer3.hlsl");
#endif

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
#if defined(CUSTOM_RENDER_NAIVE)
		dcRenderer.Draw(&camera, &mesh);
#elif defined(CUSTOM_RENDER_PIPELINE_BINNING)
		dcRenderer.DrawPipeline(pipeline, &camera, &mesh);
#endif
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