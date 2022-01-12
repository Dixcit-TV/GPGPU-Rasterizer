#include "pch.h"
#include "Pipeline.h"

#include <DirectXColors.h>

#include "../../Mesh/CompuMesh.h"

namespace CompuRaster
{
	Pipeline::Pipeline()
		: m_pGeometrySetupShader{ nullptr }
		, m_pBinningShader{ nullptr }
		, m_pCoarseShader{ nullptr }
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
		Helpers::SafeRelease(m_pBinBuffer);
		Helpers::SafeRelease(m_pBinSRV);
		Helpers::SafeRelease(m_pBinUAV);
		Helpers::SafeRelease(m_pTileBuffer);
		Helpers::SafeRelease(m_pTileSRV);
		Helpers::SafeRelease(m_pTileUAV);

		Helpers::SafeRelease(m_pBinCounter);
		Helpers::SafeRelease(m_pBinCounterUAV);

		Helpers::SafeRelease(m_pTileCounter);
		Helpers::SafeRelease(m_pTileCounterUAV);

		Helpers::SafeDelete(m_pGeometrySetupShader);
		Helpers::SafeDelete(m_pBinningShader);
		Helpers::SafeDelete(m_pCoarseShader);
		Helpers::SafeDelete(m_pFineShader);
	}

	void Pipeline::Init(ID3D11Device* pdevice, UINT vCount, UINT triangleCount, const wchar_t* geometrySetupPath, const wchar_t* binningPath, const wchar_t* tilePath, const wchar_t* finePath)
	{
		m_pGeometrySetupShader = new ComputeShader(pdevice, geometrySetupPath);
		m_pBinningShader = new ComputeShader(pdevice, binningPath);
		m_pCoarseShader = new ComputeShader(pdevice, tilePath);
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

		// Hard code for now but matches the 16*16 tile bin, for 8*8 pixel tiles, full HD resolution (ceil(1920.f / (16 * 8)), ceil(1080.f / (16 * 8)))
		const UINT binCount{ 15 * 9 };
		// I am using the vehicle from graphic programming 1 so a bit over 11k triangles -> 1,485,000+ elements
		UINT elemCount = binCount * triangleCount; // triangleCount = 11638

#pragma region INPUT_BUFFER
		D3D11_BUFFER_DESC binBufferDesc{};
		binBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		binBufferDesc.ByteWidth = elemCount * 4;
		binBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		binBufferDesc.CPUAccessFlags = 0;
		binBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		binBufferDesc.StructureByteStride = 0;

		res = pdevice->CreateBuffer(&binBufferDesc, nullptr, &m_pBinBuffer);
		if (FAILED(res))
			return;

		D3D11_SHADER_RESOURCE_VIEW_DESC binViewDesc{};
		binViewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		binViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		binViewDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		binViewDesc.BufferEx.FirstElement = 0;
		binViewDesc.BufferEx.NumElements = elemCount;
		res = pdevice->CreateShaderResourceView(m_pBinBuffer, &binViewDesc, &m_pBinSRV);
		if (FAILED(res))
			return;

		D3D11_UNORDERED_ACCESS_VIEW_DESC binUavDesc{};
		binUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		binUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		binUavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		binUavDesc.Buffer.FirstElement = 0;
		binUavDesc.Buffer.NumElements = elemCount;
		res = pdevice->CreateUnorderedAccessView(m_pBinBuffer, &binUavDesc, &m_pBinUAV);
		if (FAILED(res))
			return;
#pragma endregion

		// Ouput buffer structure is a uint2x4, so 8 * 4 bytes stride, used as a bit mask for the 16*16 tiles (256 bits, 256 tiles)
		const UINT tileStride = 4 * 8; 
		D3D11_BUFFER_DESC tileBufferDesc{ };
		tileBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		tileBufferDesc.ByteWidth = elemCount * tileStride; // 11mill + so no overflow, and close to 12 MB
		tileBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		tileBufferDesc.CPUAccessFlags = 0;
		tileBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		tileBufferDesc.StructureByteStride = tileStride;
		res = pdevice->CreateBuffer(&tileBufferDesc, nullptr, &m_pTileBuffer);
		if (FAILED(res))
			return;

		D3D11_SHADER_RESOURCE_VIEW_DESC tileViewDesc{ };
		tileViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		tileViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		tileViewDesc.Buffer.FirstElement = 0;
		tileViewDesc.Buffer.NumElements = elemCount;
		res = pdevice->CreateShaderResourceView(m_pTileBuffer, &tileViewDesc, &m_pTileSRV);
		if (FAILED(res))
			return;

		D3D11_UNORDERED_ACCESS_VIEW_DESC tileUavDesc{ };
		tileUavDesc.Format = DXGI_FORMAT_UNKNOWN;
		tileUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		tileUavDesc.Buffer.Flags = 0;
		tileUavDesc.Buffer.FirstElement = 0;
		tileUavDesc.Buffer.NumElements = elemCount;
		res = pdevice->CreateUnorderedAccessView(m_pTileBuffer, &tileUavDesc, &m_pTileUAV);
		if (FAILED(res))
			return;


		D3D11_BUFFER_DESC counterDesc{};
		counterDesc.Usage = D3D11_USAGE_DEFAULT;
		counterDesc.ByteWidth = 4;
		counterDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		counterDesc.CPUAccessFlags = 0;
		counterDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		counterDesc.StructureByteStride = 0;

		UINT null = 0;
		D3D11_SUBRESOURCE_DATA counterData;
		counterData.pSysMem = &null;

		res = pdevice->CreateBuffer(&counterDesc, &counterData, &m_pTileCounter);
		if (FAILED(res))
			return;

		D3D11_UNORDERED_ACCESS_VIEW_DESC counterUavDesc{};
		counterUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		counterUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		counterUavDesc.Buffer.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		counterUavDesc.Buffer.FirstElement = 0;
		counterUavDesc.Buffer.NumElements = 1;
		res = pdevice->CreateUnorderedAccessView(m_pTileCounter, &counterUavDesc, &m_pTileCounterUAV);
		if (FAILED(res))
			return;

		res = pdevice->CreateBuffer(&counterDesc, &counterData, &m_pBinCounter);
		if (FAILED(res))
			return;

		res = pdevice->CreateUnorderedAccessView(m_pBinCounter, &counterUavDesc, &m_pBinCounterUAV);
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
		pdeviceContext->Dispatch(24, 1, 1);

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

		////FINE SHADER
		//pdeviceContext->CSSetShader(m_pFineShader->GetShader(), nullptr, 0);

		//ID3D11ShaderResourceView* fineSrvs[]{ m_pRasterDataSRV, m_pBinSRV, pmesh->GetVertexOutBufferView(), pmesh->GetIndexBufferView() };
		//pdeviceContext->CSSetShaderResources(0, 4, fineSrvs);
		//pdeviceContext->Dispatch(30, 17, 1);
		////pdeviceContext->Dispatch(4, 4, 1);
		//ID3D11ShaderResourceView* nullSrvs4[]{ nullptr, nullptr, nullptr, nullptr };
		//pdeviceContext->CSSetShaderResources(0, 4, nullSrvs4);

		//pdeviceContext->ClearUnorderedAccessViewUint(m_pBinUAV, reinterpret_cast<const UINT*>(&DirectX::Colors::Black));

		//BIN SHADER
		pdeviceContext->CSSetShader(m_pBinningShader->GetShader(), nullptr, 0);
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, &m_pBinUAV, nullptr);
		pdeviceContext->CSSetShaderResources(0, 1, &m_pRasterDataSRV);
		pdeviceContext->Dispatch(24, 1, 1);

		pdeviceContext->CSSetUnorderedAccessViews(2, 1, nullUav, nullptr);
		ID3D11ShaderResourceView* nullSrvs[]{ nullptr };
		pdeviceContext->CSSetShaderResources(0, 1, nullSrvs);

		//TILE SHADER
		pdeviceContext->CSSetShader(m_pCoarseShader->GetShader(), nullptr, 0);

		ID3D11UnorderedAccessView* tileUavs[]{ m_pBinCounterUAV, m_pTileUAV };
		pdeviceContext->CSSetUnorderedAccessViews(2, 2, tileUavs, nullptr);

		ID3D11ShaderResourceView* tileSrvs[]{ m_pBinSRV, m_pRasterDataSRV };
		pdeviceContext->CSSetShaderResources(0, 2, tileSrvs);
		pdeviceContext->Dispatch(15, 9, 1);

		ID3D11UnorderedAccessView* nullUavs2[]{ nullptr, nullptr };
		pdeviceContext->CSSetUnorderedAccessViews(2, 2, nullUavs2, nullptr);
		pdeviceContext->CSSetShaderResources(0, 2, nullSrvs2);

		//FINE SHADER
		pdeviceContext->CSSetShader(m_pFineShader->GetShader(), nullptr, 0);

		ID3D11ShaderResourceView* fineSrvs[]{ m_pRasterDataSRV, m_pTileSRV, pmesh->GetVertexOutBufferView(), pmesh->GetIndexBufferView() };
		pdeviceContext->CSSetShaderResources(0, 4, fineSrvs);
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, &m_pTileCounterUAV, nullptr);
		pdeviceContext->Dispatch(240, 135, 1);
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, nullUav, nullptr);
		ID3D11ShaderResourceView* nullSrvs4[]{ nullptr, nullptr, nullptr, nullptr };
		pdeviceContext->CSSetShaderResources(0, 4, nullSrvs4);

		pdeviceContext->ClearUnorderedAccessViewUint(m_pTileCounterUAV, reinterpret_cast<const UINT*>(&DirectX::Colors::Black));
		pdeviceContext->ClearUnorderedAccessViewUint(m_pBinCounterUAV, reinterpret_cast<const UINT*>(&DirectX::Colors::Black));
		pdeviceContext->ClearUnorderedAccessViewUint(m_pBinUAV, reinterpret_cast<const UINT*>(&DirectX::Colors::Black));
		//pdeviceContext->ClearUnorderedAccessViewUint(m_pBinUAV, reinterpret_cast<const UINT*>(&DirectX::Colors::Black));
	}
}
