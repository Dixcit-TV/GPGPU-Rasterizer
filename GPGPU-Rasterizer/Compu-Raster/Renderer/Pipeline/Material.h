#pragma once
#include <map>
#include <vector>

#include "Render/Shader/Shader.h"

namespace CompuRaster
{
	class Mesh;

	struct ConstantBufferBinding
	{
		std::string name;
		UINT slotID;
	};

	class Material
	{
	public:
		explicit Material(ID3D11Device* pdevice, const wchar_t* csPath);
		~Material();

		Material(const Material&) = delete;
		Material(Material&&) noexcept = delete;
		Material& operator=(const Material&) = delete;
		Material& operator=(Material&&) noexcept = delete;

		template<typename SHADER_TYPE>
		void InitConstantBuffers(ID3D11Device* pdevice, Shader<SHADER_TYPE>* pshader, EShaderType type);
		template<typename TARGET_TYPE, typename... ARG_TYPE>
		void SetConstantBuffer(ID3D11DeviceContext* pdeviceContext, const std::string& bufferName, ARG_TYPE&&... args);

		void SetShaders(ID3D11DeviceContext* pdeviceContext, const Mesh* pmesh) const;

		ID3D11InputLayout* GetInputLayout() const { return m_InputLayout; }
		const std::vector<D3D11_INPUT_ELEMENT_DESC>& GetInputLayoutDesc() const { return m_InputLayoutDescs; }
		UINT GetInputLayoutSize() const { return m_InputLayoutSize; }

	private:
		std::map<EShaderType, std::vector<ConstantBufferBinding>> m_ShaderCBBinding;
		std::map<std::string, ID3D11Buffer*> m_ShaderCBs;
		std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputLayoutDescs;
		ID3D11InputLayout* m_InputLayout;

		Shader<ID3D11ComputeShader>* m_pPipelineShader;

		UINT m_InputLayoutSize;

		void GenerateInputLayout(ID3D11Device* pdevice);
		void SetShaderParameters(EShaderType shaderType, const std::vector<ConstantBufferBinding>& bindings, ID3D11DeviceContext* pdeviceContext) const;
	};

	template<typename SHADER_TYPE>
	void Material::InitConstantBuffers(ID3D11Device* pdevice, Shader<SHADER_TYPE>* pshader, EShaderType type)
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

			if (m_ShaderCBs[inputDesc.Name] || strcmp(inputDesc.Name, "g_Vertices") == 0 || strcmp(inputDesc.Name, "g_Indices") == 0)
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

	template<typename TARGET_TYPE, typename... ARG_TYPE>
	void Material::SetConstantBuffer(ID3D11DeviceContext* pdeviceContext, const std::string& bufferName, ARG_TYPE&&... args)
	{
		ID3D11Buffer* pbuffer{ m_ShaderCBs.find(bufferName) != m_ShaderCBs.cend() ? m_ShaderCBs[bufferName] : nullptr };
		if (!pbuffer)
			return;

		D3D11_MAPPED_SUBRESOURCE mappedResource;

		HRESULT res{ pdeviceContext->Map(pbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) };
		if (FAILED(res))
			return;

		TARGET_TYPE* dataPtr{ static_cast<TARGET_TYPE*>(mappedResource.pData) };
		*dataPtr = TARGET_TYPE{ std::forward<ARG_TYPE>(args)... };

		pdeviceContext->Unmap(pbuffer, 0);
	}
}