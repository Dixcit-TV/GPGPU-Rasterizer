#pragma once
namespace HelperStruct
{
	struct CameraVertexMatrix
	{
		DirectX::XMFLOAT4X4 worldViewProjection{};
	};

	struct CameraVertexMatricesDebug
	{
		DirectX::XMFLOAT4X4 view{};
		DirectX::XMFLOAT4X4 proj{};
		DirectX::XMFLOAT4X4 worldViewProjection{};
		DirectX::XMFLOAT4X4 world{};
	};
};

