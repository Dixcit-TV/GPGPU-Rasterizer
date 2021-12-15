#pragma once
#include <map>
#include <vector>

#include "Render/Shader/Shader.h"

class HardwareRenderer;

class Material
{
public:
	explicit Material(ID3D11Device* pdevice, const wchar_t* vsPath, const wchar_t* hsPath, const wchar_t* dsPath, const wchar_t* gsPath, const wchar_t* psPath);
	~Material();

	Material(const Material&) = delete;
	Material(Material&&) noexcept = delete;
	Material& operator=(const Material&) = delete;
	Material& operator=(Material&&) noexcept = delete;

	template<typename TARGET_TYPE>
	void InitConstantBuffer(ID3D11Device* pdevice, EShaderType type, UINT idx);
	template<typename TARGET_TYPE, typename... ARG_TYPE>
	void SetConstantBuffer(ID3D11DeviceContext* pdeviceContext, EShaderType type, UINT idx, ARG_TYPE&&... args);

	void SetConstantBufferCount(EShaderType type, UINT newCount);
	void SetShaders(ID3D11DeviceContext* pdeviceContext) const;

	ID3D11InputLayout* GetInputLayout() const { return m_InputLayout; }
	const std::vector<D3D11_INPUT_ELEMENT_DESC>& GetInputLayoutDesc() const { return m_InputLayoutDescs; }
	UINT GetInputLayoutSize() const { return m_InputLayoutSize; }

private:
	std::map<EShaderType, std::vector<ID3D11Buffer*>> m_ShaderConstantBuffers;
	std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputLayoutDescs;
	ID3D11InputLayout* m_InputLayout;

	Shader<ID3D11VertexShader>* m_pVertexShader;
	Shader<ID3D11HullShader>* m_pHullShader;
	Shader<ID3D11DomainShader>* m_pDomainShader;
	Shader<ID3D11GeometryShader>* m_pGeometryShader;
	Shader<ID3D11PixelShader>* m_pPixelShader;

	UINT m_InputLayoutSize;

	void GenerateInputLayout(ID3D11Device* pdevice);
	void SetShaderParameters(EShaderType shaderType, ID3D11DeviceContext* pdeviceContext) const;
};

template<typename TARGET_TYPE>
void Material::InitConstantBuffer(ID3D11Device* pdevice, EShaderType type, UINT idx)
{
	assert(m_ShaderConstantBuffers[type].size() > idx);

	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(TARGET_TYPE);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	const HRESULT res{ pdevice->CreateBuffer(&bufferDesc, nullptr, &m_ShaderConstantBuffers[type][idx]) };
	if (FAILED(res))
		res;
}

template<typename TARGET_TYPE, typename... ARG_TYPE>
void Material::SetConstantBuffer(ID3D11DeviceContext* pdeviceContext, EShaderType type, UINT idx, ARG_TYPE&&... args)
{
	assert(m_ShaderConstantBuffers[type].size() > idx && m_ShaderConstantBuffers[type][idx]);

	ID3D11Buffer* pbuffer{ m_ShaderConstantBuffers[type][idx] };

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Lock the constant buffer so it can be written to.
	HRESULT res{ pdeviceContext->Map(pbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) };
	if (FAILED(res))
		return;

	// Get a pointer to the data in the constant buffer.
	TARGET_TYPE* dataPtr{ static_cast<TARGET_TYPE*>(mappedResource.pData) };
	*dataPtr = TARGET_TYPE{ std::forward<ARG_TYPE>(args)... };

	pdeviceContext->Unmap(pbuffer, 0);
}