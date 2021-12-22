#include "pch.h"
#include "Mesh.h"

#include "Common/Structs.h"
#include "../Renderer/Pipeline/Material.h"
#include "Camera/Camera.h"

namespace CompuRaster
{
	Mesh::Mesh(std::vector<DirectX::XMFLOAT3>&& positions, std::vector<DirectX::XMFLOAT3>&& normals, std::vector<DirectX::XMFLOAT2>&& uvs, std::vector<uint32_t>&& indices, bool calculateTangents)
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

	Mesh::~Mesh()
	{
		Helpers::SafeRelease(m_VertexBufferView);
		Helpers::SafeRelease(m_IndexBufferView);
		Helpers::SafeRelease(m_VertexBuffer);
		Helpers::SafeRelease(m_IndexBuffer);
	}

	void Mesh::SetMaterial(ID3D11Device* pdevice, Material* pmaterial)
	{
		m_pMaterial = pmaterial;
		BuildVertexBuffer(pdevice);
		BuildIndexBuffer(pdevice);
	}

	void Mesh::BuildVertexBuffer(ID3D11Device* pdevice)
	{
		if (!m_pMaterial)
			return;

		UINT vCount{ static_cast<UINT>(std::size(m_VertexPositions)) };
		UINT vStride{ static_cast<UINT>(sizeof DirectX::XMFLOAT3 + sizeof DirectX::XMFLOAT3) };

		D3D11_BUFFER_DESC vBufferDesc{};
		vBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vBufferDesc.ByteWidth = vStride * vCount;
		vBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		vBufferDesc.CPUAccessFlags = 0;
		vBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		vBufferDesc.StructureByteStride = vStride;

		char* pdata{ new char[vBufferDesc.ByteWidth] };

		for (UINT idx{}; idx < vCount; ++idx)
		{
			UINT memOffset{ vStride * idx };
			memcpy(pdata + memOffset, &m_VertexPositions[idx], sizeof m_VertexPositions[idx]);

			if (idx < std::size(m_VertexNorms))
			{
				memOffset += static_cast<UINT>(sizeof DirectX::XMFLOAT3);
				memcpy(pdata + memOffset, &m_VertexNorms[idx], sizeof m_VertexNorms[idx]);
			}
		}

		D3D11_SUBRESOURCE_DATA vResData{};
		vResData.pSysMem = pdata;

		HRESULT res{ pdevice->CreateBuffer(&vBufferDesc, &vResData, &m_VertexBuffer) };
		if (FAILED(res))
			return;

		delete[] pdata;

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.NumElements = vCount;
		res = pdevice->CreateShaderResourceView(m_VertexBuffer, &viewDesc, &m_VertexBufferView);
		if (FAILED(res))
			return;
	}

	void Mesh::BuildIndexBuffer(ID3D11Device* pdevice)
	{
		UINT iCount{ static_cast<UINT>(std::size(m_Indices)) };
		UINT iStride{ static_cast<UINT>(sizeof uint32_t) };

		D3D11_BUFFER_DESC iBufferDesc{};
		iBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		iBufferDesc.ByteWidth = iCount * iStride;
		iBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		iBufferDesc.CPUAccessFlags = 0;
		iBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		iBufferDesc.StructureByteStride = iStride;

		D3D11_SUBRESOURCE_DATA vResData{};
		vResData.pSysMem = std::data(m_Indices);

		HRESULT res{ pdevice->CreateBuffer(&iBufferDesc, &vResData, &m_IndexBuffer) };
		if (FAILED(res))
			return;

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.NumElements = iCount;
		res = pdevice->CreateShaderResourceView(m_IndexBuffer, &viewDesc, &m_IndexBufferView);
		if (FAILED(res))
			return;
	}

	void Mesh::SetupDrawInfo(Camera* pcamera, ID3D11DeviceContext* pdeviceContext) const
	{
		if (!m_pMaterial)
			return;

		DirectX::XMFLOAT4X4 world{};
		DirectX::XMFLOAT4X4 viewProj{ pcamera->GetViewProjection() };
		DirectX::XMFLOAT4X4 worldViewProj{};

		XMStoreFloat4x4(&world, DirectX::XMMatrixIdentity());
		XMStoreFloat4x4(&worldViewProj, XMLoadFloat4x4(&viewProj));
		m_pMaterial->SetConstantBuffer<HelperStruct::CameraObjectMatrices>(pdeviceContext, "ObjectMatrices", worldViewProj, world);
		m_pMaterial->SetConstantBuffer<HelperStruct::LightInfoBuffer>(pdeviceContext, "LightInfo", DirectX::XMFLOAT3{ 0.f, -1.f, 0.f }, 2.f);
		m_pMaterial->SetShaders(pdeviceContext, this);
	}
}