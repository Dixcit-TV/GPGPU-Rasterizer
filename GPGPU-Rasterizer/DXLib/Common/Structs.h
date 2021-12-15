#pragma once
namespace HelperStruct
{
	struct CameraVertexMatrices
	{
		DirectX::XMFLOAT4X4 worldViewProjection{};
		DirectX::XMFLOAT4X4 world{};
	};
};

