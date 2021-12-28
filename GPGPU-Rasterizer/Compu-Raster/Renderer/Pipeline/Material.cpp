#include "pch.h"
#include "Material.h"
#include "Common/Helpers.h"
#include "Managers/Logger.h"

namespace CompuRaster
{
	Material::Material()
		: m_pVertexShader{ nullptr }
		, m_pPixelShader{ nullptr }
	{}

	Material::~Material()
	{
		Helpers::SafeDelete(m_pVertexShader);
		Helpers::SafeDelete(m_pPixelShader);
	}

	void Material::Init(ID3D11Device* pdevice, const wchar_t* vsPath, const wchar_t*)
	{
		m_pVertexShader = new ComputeShader(pdevice, vsPath);
		APP_LOG_ERROR(L"Could not create Vertex Shader !", m_pVertexShader);

		//m_pPixelShader = new ComputeShader(pdevice, psPath);
		//APP_LOG_ERROR(L"Could not create Pixel Shader !", m_pPixelShader);
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

			m_ShaderCBBinding[type].push_back(ConstantBufferBinding{ inputDesc.Name, inputDesc.BindPoint });

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
}
