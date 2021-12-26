#include "pch.h"
#include "NaiveMaterial.h"
#include "../../../Mesh/Mesh.h"
#include "Managers/Logger.h"

namespace CompuRaster
{

	NaiveMaterial::NaiveMaterial(ID3D11Device* pdevice, const wchar_t* csPath)
		: m_ShaderCBBinding{}
		, m_ShaderCBs{}
		, m_InputLayoutDescs{}
		, m_InputLayout{ nullptr }
		, m_pPipelineShader{ csPath != nullptr ? new Shader<ID3D11ComputeShader>(pdevice, csPath) : nullptr }
		, m_InputLayoutSize{ 0 }
	{
		APP_LOG_ERROR(L"Could not create Pipeline Shader !", m_pPipelineShader);
		//GenerateInputLayout(pdevice);
		InitConstantBuffers(pdevice, m_pPipelineShader, EShaderType::Compute);
	}

	NaiveMaterial::~NaiveMaterial()
	{
		for (auto& bufferPairs : m_ShaderCBs)
		{
			Helpers::SafeRelease(bufferPairs.second);
		}

		Helpers::SafeRelease(m_InputLayout);
		Helpers::SafeDelete(m_pPipelineShader);
	}

	void NaiveMaterial::SetShaders(ID3D11DeviceContext* pdeviceContext, const Mesh* pmesh) const
	{
		ID3D11ShaderResourceView* vBuffer{ pmesh->GetVertexBufferView() };
		ID3D11ShaderResourceView* iBuffer{ pmesh->GetIndexBufferView() };

		for (const auto& mapPair : m_ShaderCBBinding)
		{
			SetShaderParameters(mapPair.first, mapPair.second, pdeviceContext);

			for (const ConstantBufferBinding& binding : mapPair.second)
			{
				if (binding.name == "g_Vertices")
					pdeviceContext->CSSetShaderResources(binding.slotID, 1, &vBuffer);
				if (binding.name == "g_Indices")
					pdeviceContext->CSSetShaderResources(binding.slotID, 1, &iBuffer);
			}
		}

		pdeviceContext->CSSetShader(m_pPipelineShader ? m_pPipelineShader->GetShader() : nullptr, nullptr, 0);
	}

	void NaiveMaterial::SetShaderParameters(EShaderType shaderType, const std::vector<ConstantBufferBinding>& bindings, ID3D11DeviceContext* pdeviceContext) const
	{
		if (shaderType != EShaderType::Compute)
			return;

		for (const ConstantBufferBinding& binding : bindings)
		{
			pdeviceContext->CSSetConstantBuffers(binding.slotID, 1, &m_ShaderCBs.at(binding.name));
		}
	}

	void NaiveMaterial::GenerateInputLayout(ID3D11Device* pdevice)
	{
		if (!m_pPipelineShader || !m_pPipelineShader->GetShader())
			return;

		ID3D11ShaderReflection* pReflector;
		ID3DBlob* shaderBlob{ m_pPipelineShader->GetShaderBlob() };
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
}
