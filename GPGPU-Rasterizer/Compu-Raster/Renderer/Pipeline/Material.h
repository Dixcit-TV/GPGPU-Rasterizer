#pragma once
#include <map>
#include <string>
#include <vector>
#include "Render/Shader/Shader.h"

enum class EShaderType;
namespace CompuRaster
{
	class CompuMesh;

	struct CBufferBinding
	{
		std::string name;
		UINT slotID;
	};

	class Material
	{
	public:
		explicit Material();
		~Material();

		Material(const Material&) = delete;
		Material(Material&&) noexcept = delete;
		Material& operator=(const Material&) = delete;
		Material& operator=(Material&&) noexcept = delete;

		void Init(ID3D11Device* pdevice, const wchar_t* vsPath, const wchar_t* psPath);

		ComputeShader* GetVertexShader() const { return m_pVertexShader; }
		ComputeShader* GetPixelShader() const { return m_pPixelShader; }

		void InitConstantBuffers(ID3D11Device* pdevice, ComputeShader* pshader, EShaderType type);
		template<typename TARGET_TYPE, typename... ARG_TYPE>
		void SetConstantBuffer(ID3D11DeviceContext* pdeviceContext, const std::string& bufferName, ARG_TYPE&&... args);

		void SetShaders(ID3D11DeviceContext* pdeviceContext, const CompuMesh* pmesh) const;

	private:
		std::map<EShaderType, std::vector<CBufferBinding>> m_ShaderCBBinding;
		std::map<std::string, ID3D11Buffer*> m_ShaderCBs;

		ComputeShader* m_pVertexShader;
		ComputeShader* m_pPixelShader;

		//std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputLayoutDescs;

		//UINT m_InputLayoutSize;

		void SetShaderParameters(EShaderType shaderType, const std::vector<CBufferBinding>& bindings, ID3D11DeviceContext* pdeviceContext) const;
	};

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

