#pragma once

class TriangleMesh
{
public:
	explicit TriangleMesh(std::vector<DirectX::XMFLOAT3>&& positions, std::vector<DirectX::XMFLOAT3>&& normals, std::vector<DirectX::XMFLOAT2>&& uvs, std::vector<uint32_t>&& indices, bool calculateTangents = false);

private:
	DirectX::XMFLOAT4X4 m_WorldMatrix;

	std::vector<DirectX::XMFLOAT3> m_VertexPositions;
	std::vector<DirectX::XMFLOAT3> m_VertexNorms;
	std::vector<DirectX::XMFLOAT3> m_VertexTangents;
	std::vector<DirectX::XMFLOAT2> m_VertexUvs;
	std::vector<uint32_t> m_Indices;
};

