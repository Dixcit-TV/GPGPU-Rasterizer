#include "pch.h"
#include "Material.h"

Material::Material(ID3D11Device* pdevice, const wchar_t* vsPath, const wchar_t* hsPath, const wchar_t* dsPath, const wchar_t* gsPath, const wchar_t* psPath)
	: m_ShaderCBBinding{}
	, m_ShaderCBs{}
	, m_InputLayoutDescs{}
	, m_InputLayout{ nullptr }
	, m_pVertexShader{ vsPath != nullptr ? new Shader<ID3D11VertexShader>(pdevice, vsPath) : nullptr }
	, m_pHullShader{ hsPath != nullptr ? new Shader<ID3D11HullShader>(pdevice, hsPath) : nullptr }
	, m_pDomainShader{ dsPath != nullptr ? new Shader<ID3D11DomainShader>(pdevice, dsPath) : nullptr }
	, m_pGeometryShader{ gsPath != nullptr ? new Shader<ID3D11GeometryShader>(pdevice, gsPath) : nullptr }
	, m_pPixelShader{ psPath != nullptr ? new Shader<ID3D11PixelShader>(pdevice, psPath) : nullptr }
	, m_InputLayoutSize{ 0 }
{
	GenerateInputLayout(pdevice);
	InitConstantBuffers(pdevice, m_pVertexShader, EShaderType::Vertex);
	InitConstantBuffers(pdevice, m_pHullShader, EShaderType::Hull);
	InitConstantBuffers(pdevice, m_pDomainShader, EShaderType::Domain);
	InitConstantBuffers(pdevice, m_pGeometryShader, EShaderType::Geometry);
	InitConstantBuffers(pdevice, m_pPixelShader, EShaderType::Pixel);
}

Material::~Material()
{
	for (auto& bufferPairs : m_ShaderCBs)
	{
		Helpers::SafeRelease(bufferPairs.second);
	}

	Helpers::SafeRelease(m_InputLayout);
	Helpers::SafeDelete(m_pVertexShader);
	Helpers::SafeDelete(m_pHullShader);
	Helpers::SafeDelete(m_pDomainShader);
	Helpers::SafeDelete(m_pGeometryShader);
	Helpers::SafeDelete(m_pPixelShader);
}

void Material::SetShaders(ID3D11DeviceContext* pdeviceContext) const
{
	for (const auto& mapPair : m_ShaderCBBinding)
	{
		SetShaderParameters(mapPair.first, mapPair.second, pdeviceContext);
	}

	pdeviceContext->IASetInputLayout(m_InputLayout);
	pdeviceContext->VSSetShader(m_pVertexShader ? m_pVertexShader->GetShader() : nullptr, nullptr, 0);
	pdeviceContext->HSSetShader(m_pHullShader ? m_pHullShader->GetShader() : nullptr, nullptr, 0);
	pdeviceContext->DSSetShader(m_pDomainShader ? m_pDomainShader->GetShader() : nullptr, nullptr, 0);
	pdeviceContext->GSSetShader(m_pGeometryShader ? m_pGeometryShader->GetShader() : nullptr, nullptr, 0);
	pdeviceContext->PSSetShader(m_pPixelShader ? m_pPixelShader->GetShader() : nullptr, nullptr, 0);
}

void Material::SetShaderParameters(EShaderType shaderType, const std::vector<ConstantBufferBinding>& bindings, ID3D11DeviceContext* pdeviceContext) const
{
	for (const ConstantBufferBinding& binding : bindings)
	{
		switch (shaderType)
		{
		case EShaderType::Vertex:
			pdeviceContext->VSSetConstantBuffers(binding.slotID, 1, &m_ShaderCBs.at(binding.name));
			break;
		case EShaderType::Hull:
			pdeviceContext->HSSetConstantBuffers(binding.slotID, 1, &m_ShaderCBs.at(binding.name));
			break;
		case EShaderType::Domain:
			pdeviceContext->DSSetConstantBuffers(binding.slotID, 1, &m_ShaderCBs.at(binding.name));
			break;
		case EShaderType::Geometry:
			pdeviceContext->GSSetConstantBuffers(binding.slotID, 1, &m_ShaderCBs.at(binding.name));
			break;
		case EShaderType::Pixel:
			pdeviceContext->PSSetConstantBuffers(binding.slotID, 1, &m_ShaderCBs.at(binding.name));
			break;
		default: break;
		}
	}
}

void Material::GenerateInputLayout(ID3D11Device* pdevice)
{
	if (!m_pVertexShader || !m_pVertexShader->GetShader())
		return;

	ID3D11ShaderReflection* pReflector;
	ID3DBlob* shaderBlob{ m_pVertexShader->GetShaderBlob() };
	HRESULT res{ D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(&pReflector)) };

	if (FAILED(res))
		return;

	D3D11_SHADER_DESC shaderDesc;
	pReflector->GetDesc(&shaderDesc);

	UINT byteOffset = 0;
	for (UINT idx{}; idx < shaderDesc.InputParameters; ++idx)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		pReflector->GetInputParameterDesc(idx, &paramDesc);

		// create input element desc
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = byteOffset;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;
		
		byteOffset += static_cast<UINT>(floor(log(paramDesc.Mask) / log(2)) + 1) * 4;

		// determine DXGI format
		switch (paramDesc.ComponentType)
		{
		case D3D_REGISTER_COMPONENT_FLOAT32:
			if (paramDesc.Mask == 1) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			else if (paramDesc.Mask == 3) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			else if (paramDesc.Mask == 7) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			else elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			break;
		case D3D_REGISTER_COMPONENT_UINT32:
			if (paramDesc.Mask == 1) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.Mask == 3) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.Mask == 7) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			break;
		case D3D_REGISTER_COMPONENT_SINT32:
			if (paramDesc.Mask == 1) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.Mask == 3) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.Mask == 7) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			break;
		default:
			return;
		}

		//save element desc
		m_InputLayoutDescs.push_back(elementDesc);
	}

	m_InputLayoutSize = byteOffset;

	res = pdevice->CreateInputLayout(
		std::data(m_InputLayoutDescs)
		, static_cast<UINT>(std::size(m_InputLayoutDescs))
		, shaderBlob->GetBufferPointer()
		, shaderBlob->GetBufferSize()
		, &m_InputLayout);

	if (FAILED(res))
		res;
}