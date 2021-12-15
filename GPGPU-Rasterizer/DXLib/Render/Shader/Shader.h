#pragma once
#include <string>
#include "../../Common/Helpers.h"

enum class EShaderType
{
	Vertex, Hull, Domain, Geometry, Pixel, Compute
};

template<typename SHADER_TYPE>
class Shader
{
	using ShaderCreationFnc = HRESULT(ID3D11Device::*)(const void*, SIZE_T, ID3D11ClassLinkage*, SHADER_TYPE**);
public:
	explicit Shader(const wchar_t* filePath, const char* entryPoint = nullptr);
	~Shader();

	Shader(const Shader&) = delete;
	Shader(Shader&&) noexcept = delete;
	Shader& operator=(const Shader&) = delete;
	Shader& operator=(Shader&&) noexcept = delete;

	SHADER_TYPE* GetShader() const { return m_Shader; }

private:
	SHADER_TYPE* m_Shader;

	HRESULT Init(const wchar_t* filePath, const char* entryPoint);
};

template<typename SHADER_TYPE>
Shader<SHADER_TYPE>::Shader(const wchar_t* filePath, const char* entryPoint)
	: m_Shader{ nullptr }
{
	const HRESULT res{ Init(filePath, entryPoint) };
	if (FAILED(res))
		res;
}

template<typename SHADER_TYPE>
Shader<SHADER_TYPE>::~Shader()
{
	Helpers::SafeRelease(m_Shader);
}

template<typename SHADER_TYPE>
HRESULT Shader<SHADER_TYPE>::Init(const wchar_t* filePath, const char* entryPoint)
{
	ID3D11Device* pdevice{ nullptr };
	ID3DBlob* pshaderBlob{ nullptr };
	ID3DBlob* perrorBlob{ nullptr };

	UINT flags{ 0 };
#if defined(DEBUG) || defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG;
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif 

	HRESULT res{ E_INVALIDARG };

	std::string target{};
	ShaderCreationFnc shaderFnc{};

	if constexpr (std::is_same_v<SHADER_TYPE, ID3D11VertexShader>)
	{
		target = "ps_5_0";
		shaderFnc = &ID3D11Device::CreateVertexShader;
	}
	else if constexpr (std::is_same_v<SHADER_TYPE, ID3D11HullShader>)
	{
		target = "hs_5_0";
		shaderFnc = &ID3D11Device::CreateHullShader;
	}
	else if constexpr (std::is_same_v<SHADER_TYPE, ID3D11DomainShader>)
	{
		target = "ds_5_0";
		shaderFnc = &ID3D11Device::CreateDomainShader;
	}
	else if constexpr (std::is_same_v<SHADER_TYPE, ID3D11GeometryShader>)
	{
		target = "gs_5_0";
		shaderFnc = &ID3D11Device::CreateGeometryShader;
	}
	else if constexpr (std::is_same_v<SHADER_TYPE, ID3D11PixelShader>)
	{
		target = "ps_5_0";
		shaderFnc = &ID3D11Device::CreatePixelShader;
	}
	else if constexpr (std::is_same_v<SHADER_TYPE, ID3D11ComputeShader>)
	{
		target = "cs_5_0";
		shaderFnc = &ID3D11Device::CreateComputeShader;
	}
	else
	{
		return res;
	}

	res = D3DCompileFromFile(filePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target.c_str(), flags, 0, &pshaderBlob, &perrorBlob);

	if (SUCCEEDED(res))
		res = (pdevice->*shaderFnc)(pshaderBlob->GetBufferPointer(), pshaderBlob->GetBufferSize(), nullptr, &m_Shader);

	return res;
}
