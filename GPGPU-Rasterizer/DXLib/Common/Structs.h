#pragma once
namespace HelperStruct
{
	struct CameraVertexMatrix
	{
		DirectX::XMFLOAT4X4 worldViewProjection{};
	};

	struct CameraObjectMatrices
	{
		DirectX::XMFLOAT4X4 worldViewProjection{};
		DirectX::XMFLOAT4X4 world{};
	};

	struct CameraObjectMatricesAndInfo
	{
		DirectX::XMFLOAT4X4 worldViewProjection{};
		DirectX::XMFLOAT4X4 world{};
		UINT vertexCount{};
		UINT triangleCount{};
		UINT indexCount{};
	};

	struct LightInfoBuffer
	{
		DirectX::XMFLOAT3 direction{};
		float intensity{};
	};

	struct CameraVertexMatricesDebug
	{
		DirectX::XMFLOAT4X4 view{};
		DirectX::XMFLOAT4X4 proj{};
		DirectX::XMFLOAT4X4 worldViewProjection{};
		DirectX::XMFLOAT4X4 world{};
	};
};

