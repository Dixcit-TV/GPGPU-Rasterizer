#include "pch.h"
#include "Material.h"
#include "Common/Helpers.h"
#include "Managers/Logger.h"
#include "../../Mesh/CompuMesh.h"

namespace CompuRaster
{
	Material::Material()
		: m_pVertexShader{ nullptr }
		, m_pPixelShader{ nullptr }
	{}

	Material::~Material()
	{
		for (auto& bufferPairs : m_ShaderCBs)
		{
			Helpers::SafeRelease(bufferPairs.second);
		}

		Helpers::SafeDelete(m_pVertexShader);
		Helpers::SafeDelete(m_pPixelShader);
	}

	void Material::Init(ID3D11Device* pdevice, const wchar_t* vsPath, const wchar_t*)
	{
		m_pVertexShader = new ComputeShader(pdevice, vsPath);
		APP_ASSERT_ERROR(L"Could not create Vertex Shader !", m_pVertexShader);

		//m_pPixelShader = new ComputeShader(pdevice, psPath);
		//APP_ASSERT_ERROR(L"Could not create Pixel Shader !", m_pPixelShader);

		InitConstantBuffers(pdevice, m_pVertexShader, EShaderType::Compute);
	}

	void Material::InitConstantBuffers(ID3D11Device* pdevice, ComputeShader* pshader, EShaderType type)
	{
		if (!pshader)
			return;

		ID3D11ShaderReflection* pReflector;
		ID3DBlob* shaderBlob{ pshader->GetShaderBlob() };
		HRESULT res{ D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(&pReflector)) };

		if (FAILED(res))
			return;

		D3D11_SHADER_DESC shaderDesc;
		pReflector->GetDesc(&shaderDesc);

		for (UINT idx{}; idx < shaderDesc.BoundResources; ++idx)
		{
			D3D11_SHADER_INPUT_BIND_DESC inputDesc{};
			if (FAILED(pReflector->GetResourceBindingDesc(idx, &inputDesc)))
				continue;

			ID3D11ShaderReflectionConstantBuffer* psrConstantBuffer{ pReflector->GetConstantBufferByName(inputDesc.Name) };
			D3D11_SHADER_BUFFER_DESC cbDesc{};
			if (!psrConstantBuffer
				|| FAILED(psrConstantBuffer->GetDesc(&cbDesc)))
				continue;

			m_ShaderCBBinding[type].push_back(CBufferBinding{ inputDesc.Name, inputDesc.BindPoint });

			if (m_ShaderCBs[inputDesc.Name] || strcmp(inputDesc.Name, "G_VERTEX_BUFFER") == 0 || strcmp(inputDesc.Name, "G_TRANS_VERTEX_BUFFER") == 0)
				continue;

			D3D11_BUFFER_DESC bufferDesc{};
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.ByteWidth = cbDesc.Size;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;

			res = pdevice->CreateBuffer(&bufferDesc, nullptr, &m_ShaderCBs[inputDesc.Name]);
			if (FAILED(res))
				res;
		}
	}

	void Material::SetShaders(ID3D11DeviceContext* pdeviceContext, const CompuMesh* pmesh) const
	{
		ID3D11ShaderResourceView* vBuffer{ pmesh->GetVertexBufferView() };

		for (const auto& mapPair : m_ShaderCBBinding)
		{
			SetShaderParameters(mapPair.first, mapPair.second, pdeviceContext);

			for (const CBufferBinding& binding : mapPair.second)
			{
				if (binding.name == "G_VERTEX_BUFFER")
					pdeviceContext->CSSetShaderResources(binding.slotID, 1, &vBuffer);
			}
		}

		pdeviceContext->CSSetShader(m_pVertexShader ? m_pVertexShader->GetShader() : nullptr, nullptr, 0);
	}

	void Material::SetShaderParameters(EShaderType shaderType, const std::vector<CBufferBinding>& bindings, ID3D11DeviceContext* pdeviceContext) const
	{
		if (shaderType != EShaderType::Compute)
			return;

		for (const CBufferBinding& binding : bindings)
		{
			pdeviceContext->CSSetConstantBuffers(binding.slotID, 1, &m_ShaderCBs.at(binding.name));
		}
	}
}
