#include "pch.h"
#include "Pipeline.h"
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
		Helpers::SafeDelete(m_pGeometrySetupShader);
		Helpers::SafeDelete(m_pBinningShader);
		Helpers::SafeDelete(m_pFineShader);
	}

	void Pipeline::Init(ID3D11Device* pdevice, const wchar_t* geometrySetupPath, const wchar_t* binningPath, const wchar_t* finePath)
	{
		m_pGeometrySetupShader = new ComputeShader(pdevice, geometrySetupPath);
		m_pBinningShader = new ComputeShader(pdevice, binningPath);
		m_pFineShader = new ComputeShader(pdevice, finePath);
	}

	void Pipeline::Dispatch(ID3D11DeviceContext* pdeviceContext, CompuMesh* pmesh, Camera* pcamera) const
	{
		pmesh->SetupDrawInfo(pcamera, pdeviceContext);
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, &m_pVOutoutUAV, nullptr);
		pdeviceContext->Dispatch(64, 1, 1);
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, { nullptr }, nullptr);

		//GEOMETRY SETUP SHADER
		pdeviceContext->CSSetShader(m_pGeometrySetupShader->GetShader(), nullptr, 0);
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, &m_pRasterDataUAV, nullptr);

		ID3D11ShaderResourceView* geoSrvs[]{ m_pVOutoutSRV, pmesh->GetIndexBufferView() };
		pdeviceContext->CSSetShaderResources(0, 2, geoSrvs);
		pdeviceContext->Dispatch(64, 1, 1);

		pdeviceContext->CSSetUnorderedAccessViews(2, 1, { nullptr }, nullptr);
		ID3D11ShaderResourceView* nullSrvs[]{ nullptr, nullptr };
		pdeviceContext->CSSetShaderResources(0, 2, nullSrvs);

		//BIN SHADER
		pdeviceContext->CSSetShader(m_pBinningShader->GetShader(), nullptr, 0);
		pdeviceContext->CSSetUnorderedAccessViews(2, 1, &m_pBinUAV, nullptr);
		pdeviceContext->CSSetShaderResources(0, 1, &m_pRasterDataSRV);
		pdeviceContext->Dispatch(64, 1, 1);

		pdeviceContext->CSSetUnorderedAccessViews(2, 1, { nullptr }, nullptr);
		pdeviceContext->CSSetShaderResources(0, 1, { nullptr });

		//FINE SHADER
		pdeviceContext->CSSetShader(m_pFineShader->GetShader(), nullptr, 0);

		ID3D11ShaderResourceView* fineSrvs[]{ m_pRasterDataSRV, m_pBinSRV };
		pdeviceContext->CSSetShaderResources(0, 2, fineSrvs);
		pdeviceContext->Dispatch(120, 68, 1);
	}
}
