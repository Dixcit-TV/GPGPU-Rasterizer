#include "pch.h"
#include "Pipeline.h"

#include <DirectXColors.h>

#include "../../Mesh/CompuMesh.h"

namespace CompuRaster
{
	Pipeline::Pipeline()
		: m_pGeometrySetupShader{ nullptr }
		, m_pBinningShader{ nullptr }
		, m_pFineShader{ nullptr }
	{}

	Pipeline::~Pipeline()
	{
		Helpers::SafeRelease(m_pVOutoutBuffer);
		Helpers::SafeRelease(m_pVOutoutSRV);
		Helpers::SafeRelease(m_pVOutoutUAV);
		Helpers::SafeRelease(m_pRasterDataBuffer);
		Helpers::SafeRelease(m_pRasterDataSRV);
		Helpers::SafeRelease(m_pRasterDataUAV);
		Helpers::SafeRelease(m_pBinTexture);
		Helpers::SafeRelease(m_pBinSRV);
		Helpers::SafeRelease(m_pBinUAV);

		Helpers::SafeDelete(m_pGeometrySetupShader);
		Helpers::SafeDelete(m_pBinningShader);
		Helpers::SafeDelete(m_pFineShader);
	}

	void Pipeline::Init(ID3D11Device* pdevice, UINT vCount, UINT triangleCount, const wchar_t* geometrySetupPath, const wchar_t* binningPath, const wchar_t* finePath)
	{
		m_pGeometrySetupShader = new ComputeShader(pdevice, geometrySetupPath);
		m_pBinningShader = new ComputeShader(pdevice, binningPath);
		m_pFineShader = new ComputeShader(pdevice, finePath);

		vCount;
		UINT rasterDataStride{ 4u * 16u };

		D3D11_BUFFER_DESC rasterDataBufferDesc{};
		rasterDataBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		rasterDataBufferDesc.ByteWidth = rasterDataStride * triangleCount;
		rasterDataBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		rasterDataBufferDesc.CPUAccessFlags = 0;
		rasterDataBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		rasterDataBufferDesc.StructureByteStride = rasterDataStride;

		HRESULT res{ pdevice->CreateBuffer(&rasterDataBufferDesc, nullptr, &m_pRasterDataBuffer) };
		if (FAILED(res))
			return;

		D3D11_SHADER_RESOURCE_VIEW_DESC rdViewDesc{};
		rdViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		rdViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		rdViewDesc.Buffer.FirstElement = 0;
		rdViewDesc.Buffer.NumElements = triangleCount;
		res = pdevice->CreateShaderResourceView(m_pRasterDataBuffer, &rdViewDesc, &m_pRasterDataSRV);
		if (FAILED(res))
			return;

		D3D11_UNORDERED_ACCESS_VIEW_DESC rdUavDesc{};
		rdUavDesc.Format = DXGI_FORMAT_UNKNOWN;
		rdUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		rdUavDesc.Buffer.Flags = 0;
		rdUavDesc.Buffer.FirstElement = 0;
		rdUavDesc.Buffer.NumElements = triangleCount;
		res = pdevice->CreateUnorderedAccessView(m_pRasterDataBuffer, &rdUavDesc, &m_pRasterDataUAV);
		if (FAILED(res))
			return;

		//D3D11_TEXTURE2D_DESC binTextureDesc{};
		//binTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		//binTextureDesc.Format = DXGI_FORMAT_R32_UINT;
		//binTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		//binTextureDesc.CPUAccessFlags = 0;
		//binTextureDesc.MiscFlags = 0;
		//binTextureDesc.Height = 68 * 120;
		//binTextureDesc.Width = vCount;//static_cast<UINT>(ceilf(vCount / 32.f));
		//binTextureDesc.MipLevels = 1;
		//binTextureDesc.ArraySize = 1;
		//binTextureDesc.SampleDesc.Count = 1;
		//binTextureDesc.SampleDesc.Quality = 0;

		const UINT elemCount{ 30 * 17 * triangleCount };

		D3D11_BUFFER_DESC binBufferDesc{};
		binBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		binBufferDesc.ByteWidth = elemCount * 4;
		binBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		binBufferDesc.CPUAccessFlags = 0;
		binBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		binBufferDesc.StructureByteStride = 0;

		res = pdevice->CreateBuffer(&binBufferDesc, nullptr, &m_pBinTexture);
		if (FAILED(res))
			return;

		D3D11_SHADER_RESOURCE_VIEW_DESC binViewDesc{};
		binViewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		binViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		binViewDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		binViewDesc.BufferEx.FirstElement = 0;
		binViewDesc.BufferEx.NumElements = elemCount;
		res = pdevice->CreateShaderResourceView(m_pBinTexture, &binViewDesc, &m_pBinSRV);
		if (FAILED(res))
			return;

		D3D11_UNORDERED_ACCESS_VIEW_DESC binUavDesc{};
		binUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		binUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		binUavDesc.Buffer.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		binUavDesc.Buffer.FirstElement = 0;
		binUavDesc.Buffer.NumElements = elemCount;
		res = pdevice->CreateUnorderedAccessView(m_pBinTexture, &binUavDesc, &m_pBinUAV);
		if (FAILED(res))
			return;
	}

	void Pipeline::Dispatch(ID3D11DeviceContext* pdeviceContext, CompuMesh* pmesh, Camera* pcamera) const
	{
		pmesh->SetupDrawInfo(pcamera, pdeviceContext);
		ID3D11UnorderedAccessView* outUAV{ pmesh->GetVertexOutBufferUAV() };
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, &outUAV, nullptr);
		pdeviceContext->Dispatch(64, 1, 1);
		ID3D11UnorderedAccessView* nullUav[] = { nullptr };
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, nullUav, nullptr);

		//GEOMETRY SETUP SHADER
		pdeviceContext->CSSetShader(m_pGeometrySetupShader->GetShader(), nullptr, 0);
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, &m_pRasterDataUAV, nullptr);

		ID3D11ShaderResourceView* geoSrvs[]{ pmesh->GetVertexOutBufferView(), pmesh->GetIndexBufferView() };
		pdeviceContext->CSSetShaderResources(0, 2, geoSrvs);
		pdeviceContext->Dispatch(64, 1, 1);

		pdeviceContext->CSSetUnorderedAccessViews(2, 1, nullUav, nullptr);
		ID3D11ShaderResourceView* nullSrvs2[]{ nullptr, nullptr };
		pdeviceContext->CSSetShaderResources(0, 2, nullSrvs2);

		//BIN SHADER
		//pdeviceContext->CSSetShader(m_pBinningShader->GetShader(), nullptr, 0);
		//pdeviceContext->CSSetUnorderedAccessViews(2, 1, &m_pBinUAV, nullptr);
		//pdeviceContext->CSSetShaderResources(0, 1, &m_pRasterDataSRV);
		//pdeviceContext->Dispatch(64, 1, 1);

		//pdeviceContext->CSSetUnorderedAccessViews(2, 1, nullUav, nullptr);
		//ID3D11ShaderResourceView* nullSrvs[]{ nullptr };
		//pdeviceContext->CSSetShaderResources(0, 1, nullSrvs);

		//FINE SHADER
		//pdeviceContext->CSSetShader(m_pFineShader->GetShader(), nullptr, 0);

		//ID3D11ShaderResourceView* fineSrvs[]{ m_pRasterDataSRV, pmesh->GetVertexOutBufferView(), pmesh->GetIndexBufferView() };
		//pdeviceContext->CSSetShaderResources(0, 3, fineSrvs);
		//pdeviceContext->Dispatch(1, 1, 1);
		////pdeviceContext->Dispatch(4, 4, 1);
		//ID3D11ShaderResourceView* nullSrvs3[]{ nullptr, nullptr, nullptr };
		//pdeviceContext->CSSetShaderResources(0, 3, nullSrvs3);

		//pdeviceContext->ClearUnorderedAccessViewUint(m_pBinUAV, reinterpret_cast<const UINT*>(&DirectX::Colors::Black));

		//BIN SHADER
		pdeviceContext->CSSetShader(m_pBinningShader->GetShader(), nullptr, 0);
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, &m_pBinUAV, nullptr);
		pdeviceContext->CSSetShaderResources(0, 1, &m_pRasterDataSRV);
		pdeviceContext->Dispatch(64, 1, 1);

		pdeviceContext->CSSetUnorderedAccessViews(2, 1, nullUav, nullptr);
		ID3D11ShaderResourceView* nullSrvs[]{ nullptr };
		pdeviceContext->CSSetShaderResources(0, 1, nullSrvs);

		//FINE SHADER
		pdeviceContext->CSSetShader(m_pFineShader->GetShader(), nullptr, 0);

		ID3D11ShaderResourceView* fineSrvs[]{ m_pRasterDataSRV, m_pBinSRV, pmesh->GetVertexOutBufferView(), pmesh->GetIndexBufferView() };
		pdeviceContext->CSSetShaderResources(0, 4, fineSrvs);
		pdeviceContext->Dispatch(30, 17, 1);
		ID3D11ShaderResourceView* nullSrvs4[]{ nullptr, nullptr, nullptr, nullptr };
		pdeviceContext->CSSetShaderResources(0, 4, nullSrvs4);

		//pdeviceContext->ClearUnorderedAccessViewUint(m_pBinUAV, reinterpret_cast<const UINT*>(&DirectX::Colors::Black));
	}
}
