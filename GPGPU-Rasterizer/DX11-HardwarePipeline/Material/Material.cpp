#include "pch.h"
#include "Material.h"

Material::Material(const wchar_t* vsPath, const wchar_t* hsPath, const wchar_t* dsPath, const wchar_t* gsPath, const wchar_t* psPath)
	: m_pVertexShader{ vsPath != nullptr ? new Shader<ID3D11VertexShader>(vsPath) : nullptr }
	, m_pHullShader{ hsPath != nullptr ? new Shader<ID3D11HullShader>(hsPath) : nullptr }
	, m_pDomainShader{ dsPath != nullptr ? new Shader<ID3D11DomainShader>(dsPath) : nullptr }
	, m_pGeometryShader{ gsPath != nullptr ? new Shader<ID3D11GeometryShader>(gsPath) : nullptr }
	, m_pPixelShader{ psPath != nullptr ? new Shader<ID3D11PixelShader>(psPath) : nullptr }
{}

Material::~Material()
{
	Helpers::SafeDelete(m_pVertexShader);
	Helpers::SafeDelete(m_pHullShader);
	Helpers::SafeDelete(m_pDomainShader);
	Helpers::SafeDelete(m_pGeometryShader);
	Helpers::SafeDelete(m_pPixelShader);
}

void Material::InitConstantBuffer(ID3D11Device* pdevice, EShaderType type, UINT idx, const D3D11_BUFFER_DESC& bufferDesc)
{
	assert(m_ShaderConstantBuffers[type].size() > idx);

	const HRESULT res{ pdevice->CreateBuffer(&bufferDesc, nullptr, &m_ShaderConstantBuffers[type][idx]) };
	if (FAILED(res))
		res;
}

void Material::SetConstantBufferCount(EShaderType type, UINT newCount)
{
	m_ShaderConstantBuffers[type].resize(newCount);
}

void Material::SetShaders(ID3D11DeviceContext* pdeviceContext) const
{
	for (const auto mapPair : m_ShaderConstantBuffers)
	{
		SetShaderParameters(mapPair.first, pdeviceContext);
	}

	pdeviceContext->VSSetShader(m_pVertexShader ? m_pVertexShader->GetShader() : nullptr, nullptr, 0);
	pdeviceContext->HSSetShader(m_pHullShader ? m_pHullShader->GetShader() : nullptr, nullptr, 0);
	pdeviceContext->DSSetShader(m_pDomainShader ? m_pDomainShader->GetShader() : nullptr, nullptr, 0);
	pdeviceContext->GSSetShader(m_pGeometryShader ? m_pGeometryShader->GetShader() : nullptr, nullptr, 0);
	pdeviceContext->PSSetShader(m_pPixelShader ? m_pPixelShader->GetShader() : nullptr, nullptr, 0);
}

void Material::SetShaderParameters(EShaderType shaderType, ID3D11DeviceContext* pdeviceContext) const
{
	const std::vector<ID3D11Buffer*>& buffers{ m_ShaderConstantBuffers.at(shaderType) };
	if (std::empty(buffers))
		return;

	switch(shaderType)
	{
	case EShaderType::Vertex:
		pdeviceContext->VSSetConstantBuffers(0, static_cast<UINT>(std::size(buffers)), std::data(buffers));
		break;
	case EShaderType::Hull:
		pdeviceContext->HSSetConstantBuffers(0, static_cast<UINT>(std::size(buffers)), std::data(buffers));
		break;
	case EShaderType::Domain:
		pdeviceContext->DSSetConstantBuffers(0, static_cast<UINT>(std::size(buffers)), std::data(buffers));
		break;
	case EShaderType::Geometry:
		pdeviceContext->GSSetConstantBuffers(0, static_cast<UINT>(std::size(buffers)), std::data(buffers));
		break;
	case EShaderType::Pixel:
		pdeviceContext->PSSetConstantBuffers(0, static_cast<UINT>(std::size(buffers)), std::data(buffers));
		break;
	default: break;
	}
}