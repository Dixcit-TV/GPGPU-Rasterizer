#include "pch.h"
#include "TriangleMesh.h"

TriangleMesh::TriangleMesh(std::vector<DirectX::XMFLOAT3>&& positions, std::vector<DirectX::XMFLOAT3>&& normals, std::vector<DirectX::XMFLOAT2>&& uvs, std::vector<uint32_t>&& indices, bool calculateTangents)
	: m_WorldMatrix{  }
	, m_VertexPositions{ positions }
	, m_VertexNorms{ normals }
	, m_VertexTangents{  }
	, m_VertexUvs{ uvs }
	, m_Indices{ indices }
{
	XMStoreFloat4x4(&m_WorldMatrix, DirectX::XMMatrixIdentity());
	if (calculateTangents)
	{
		m_VertexTangents.resize(m_VertexPositions.size());
	}
}