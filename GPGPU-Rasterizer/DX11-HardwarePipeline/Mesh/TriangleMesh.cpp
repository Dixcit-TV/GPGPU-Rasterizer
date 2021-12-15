#include "pch.h"
#include "TriangleMesh.h"

#include "../Material/Material.h"

TriangleMesh::TriangleMesh(std::vector<DirectX::XMFLOAT3>&& positions, std::vector<DirectX::XMFLOAT3>&& normals, std::vector<DirectX::XMFLOAT2>&& uvs, std::vector<uint32_t>&& indices, bool calculateTangents)
	: m_WorldMatrix{  }
	, m_VertexPositions{ positions }
	, m_VertexNorms{ normals }
	, m_VertexTangents{  }
	, m_VertexUvs{ uvs }
	, m_Indices{ indices }
	, m_VertexBuffer{ nullptr }
	, m_IndexBuffer{ nullptr }
	, m_pMaterial{ nullptr }
{
	XMStoreFloat4x4(&m_WorldMatrix, DirectX::XMMatrixIdentity());

	calculateTangents;
	m_VertexTangents.resize(m_VertexPositions.size());
}

TriangleMesh::~TriangleMesh()
{
	Helpers::SafeRelease(m_VertexBuffer);
	Helpers::SafeRelease(m_IndexBuffer);
}

void TriangleMesh::SetMaterial(ID3D11Device* pdevice, Material* pmaterial)
{
	m_pMaterial = pmaterial;
	BuildVertexBuffer(pdevice);
	BuildIndexBuffer(pdevice);
}

void TriangleMesh::BuildVertexBuffer(ID3D11Device* pdevice)
{
	if (!m_pMaterial)
		return;

	UINT vCount{ static_cast<UINT>(m_VertexPositions.size()) };
	UINT vStride{ m_pMaterial->GetInputLayoutSize() };

	D3D11_BUFFER_DESC vBufferDesc{};
	vBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vBufferDesc.ByteWidth = vStride * vCount;
	vBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vBufferDesc.CPUAccessFlags = 0;
	vBufferDesc.MiscFlags = 0;
	vBufferDesc.StructureByteStride = 0;

	const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDescs{ m_pMaterial->GetInputLayoutDesc() };
	char* pdata{ static_cast<char*>(malloc(vBufferDesc.ByteWidth)) };

	for (UINT idx{}; idx < vCount; ++idx)
	{
		const UINT vOffset{ vStride * idx };
		for (const D3D11_INPUT_ELEMENT_DESC& desc : inputDescs)
		{
			const UINT inputOffset{ vOffset + desc.AlignedByteOffset };
			if (strcmp(desc.SemanticName, "POSITION") == 0)
				memcpy(pdata + inputOffset, &m_VertexPositions[idx], sizeof m_VertexPositions[idx]);
			else if (strcmp(desc.SemanticName, "NORMAL") == 0)
				memcpy(pdata + inputOffset, &m_VertexNorms[idx], sizeof m_VertexNorms[idx]);
			else if (strcmp(desc.SemanticName, "TANGENT") == 0)
				memcpy(pdata + inputOffset, &m_VertexTangents[idx], sizeof m_VertexTangents[idx]);
			else if (strcmp(desc.SemanticName, "TEXTCOORD") == 0)
				memcpy(pdata + inputOffset, &m_VertexUvs[idx], sizeof m_VertexUvs[idx]);
		}
	}

	D3D11_SUBRESOURCE_DATA vResData{};
	vResData.pSysMem = pdata;

	HRESULT res{ pdevice->CreateBuffer(&vBufferDesc, &vResData, &m_VertexBuffer) };
	if (FAILED(res))
		return;
}

void TriangleMesh::BuildIndexBuffer(ID3D11Device* pdevice)
{
	D3D11_BUFFER_DESC iBufferDesc{};
	iBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	iBufferDesc.ByteWidth = static_cast<UINT>(std::size(m_Indices) * sizeof uint32_t);
	iBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	iBufferDesc.CPUAccessFlags = 0;
	iBufferDesc.MiscFlags = 0;
	iBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vResData{};
	vResData.pSysMem = std::data(m_Indices);

	HRESULT res{ pdevice->CreateBuffer(&iBufferDesc, &vResData, &m_IndexBuffer) };
	if (FAILED(res))
		return;
}

void TriangleMesh::SetupDrawInfo(ID3D11DeviceContext* pdeviceContext) const
{
	if (!m_pMaterial)
		return;

	const UINT stride{ m_pMaterial->GetInputLayoutSize() };
	const UINT offset{ 0 };
	pdeviceContext->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
	pdeviceContext->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pdeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_pMaterial->SetShaders(pdeviceContext);
}