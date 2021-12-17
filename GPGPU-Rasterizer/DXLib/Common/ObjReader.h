#pragma once
namespace ObjReader
{
	void LoadModel(const std::wstring& objPath, std::vector<DirectX::XMFLOAT3>& positions, std::vector<DirectX::XMFLOAT3>& normals, std::vector<DirectX::XMFLOAT2>& uvs, std::vector<uint32_t>& indexBuffer);
	int64_t GetVertexIdx(const DirectX::XMFLOAT3& vertexPos, const std::vector<DirectX::XMFLOAT3>& vertexPositions);
};

