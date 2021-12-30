#include "pch.h"
#include "CompuMesh.h"

#include "Common/Structs.h"
#include "../Renderer/Pipeline/Material.h"
#include "Camera/Camera.h"
#include "Common/Helpers.h"

namespace CompuRaster
{
	CompuMesh::CompuMesh(std::vector<DirectX::XMFLOAT3>&& positions, std::vector<DirectX::XMFLOAT3>&& normals, std::vector<DirectX::XMFLOAT2>&& uvs, std::vector<uint32_t>&& indices, bool calculateTangents)
		: m_WorldMatrix{  }
		, m_VertexPositions{ positions }
		, m_VertexNorms{ normals }
		, m_VertexTangents{  }
		, m_VertexUvs{ uvs }
		, m_Indices{ indices }
		, m_VertexBufferView{ nullptr }
		, m_VertexOutBufferView{ nullptr }
		, m_IndexBufferView{ nullptr }
		, m_VertexOutBufferUAV{ nullptr }
		, m_VertexBuffer{ nullptr }
		, m_VertexOutBuffer{ nullptr }
		, m_IndexBuffer{ nullptr }
		, m_pMaterial{ nullptr }
	{
		XMStoreFloat4x4(&m_WorldMatrix, DirectX::XMMatrixIdentity());

		calculateTangents;
		m_VertexTangents.resize(m_VertexPositions.size());
	}

	CompuMesh::~CompuMesh()
	{
		Helpers::SafeRelease(m_VertexBufferView);
		Helpers::SafeRelease(m_VertexOutBufferView);
		Helpers::SafeRelease(m_IndexBufferView);
		Helpers::SafeRelease(m_VertexOutBufferUAV);
		Helpers::SafeRelease(m_VertexBuffer);
		Helpers::SafeRelease(m_VertexOutBuffer);
		Helpers::SafeRelease(m_IndexBuffer);
	}

	void CompuMesh::SetMaterial(ID3D11Device* pdevice, Material* pmaterial)
	{
		m_pMaterial = pmaterial;
		BuildVertexBuffer(pdevice);
		BuildIndexBuffer(pdevice);
	}

	void CompuMesh::BuildVertexBuffer(ID3D11Device* pdevice)
	{
		if (!m_pMaterial)
			return;

		UINT vCount{ static_cast<UINT>(std::size(m_VertexPositions)) };
		UINT vStride{ 32u };

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
				memOffset += 16u;
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

		UINT vOutStride{ 32u };

		vBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vBufferDesc.ByteWidth = vOutStride * vCount;
		vBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		vBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		vBufferDesc.StructureByteStride = vOutStride;

		res = pdevice->CreateBuffer(&vBufferDesc, nullptr, &m_VertexOutBuffer);
		if (FAILED(res))
			return;

		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.NumElements = vCount;
		res = pdevice->CreateShaderResourceView(m_VertexOutBuffer, &viewDesc, &m_VertexOutBufferView);
		if (FAILED(res))
			return;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.Flags = 0;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = vCount;
		res = pdevice->CreateUnorderedAccessView(m_VertexOutBuffer, &uavDesc, &m_VertexOutBufferUAV);
		if (FAILED(res))
			return;
	}

	void CompuMesh::BuildIndexBuffer(ID3D11Device* pdevice)
	{
		UINT iCount{ static_cast<UINT>(std::size(m_Indices)) };
		UINT iStride{ static_cast<UINT>(sizeof uint32_t) };

		D3D11_BUFFER_DESC iBufferDesc{};
		iBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		iBufferDesc.ByteWidth = iCount * iStride;
		iBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		iBufferDesc.CPUAccessFlags = 0;
		iBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		iBufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vResData{};
		vResData.pSysMem = std::data(m_Indices);

		HRESULT res{ pdevice->CreateBuffer(&iBufferDesc, &vResData, &m_IndexBuffer) };
		if (FAILED(res))
			return;

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
		viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		viewDesc.BufferEx.FirstElement = 0;
		viewDesc.BufferEx.NumElements = iCount;
		viewDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		res = pdevice->CreateShaderResourceView(m_IndexBuffer, &viewDesc, &m_IndexBufferView);
		if (FAILED(res))
			return;
	}

	void CompuMesh::SetupDrawInfo(Camera* pcamera, ID3D11DeviceContext* pdeviceContext) const
	{
		if (!m_pMaterial)
			return;

		DirectX::XMFLOAT4X4 world{};
		DirectX::XMFLOAT4X4 viewProj{ pcamera->GetViewProjection() };
		DirectX::XMFLOAT4X4 worldViewProj{};

		XMStoreFloat4x4(&world, DirectX::XMMatrixIdentity());
		XMStoreFloat4x4(&worldViewProj, XMLoadFloat4x4(&viewProj));
		m_pMaterial->SetConstantBuffer<HelperStruct::CameraObjectMatrices>(pdeviceContext, "ObjectInfo", worldViewProj, world);
		m_pMaterial->SetShaders(pdeviceContext, this);
	}
}
