#pragma once
#include <sstream>
#include <string>
#include "../../Common/Helpers.h"

enum class EShaderType
{
	Vertex, Hull, Domain, Geometry, Pixel, Compute
};

template<typename SHADER_TYPE>
class Shader
{
	using ShaderCreationFnc = HRESULT(__stdcall ID3D11Device::*)(const void*, SIZE_T, ID3D11ClassLinkage*, SHADER_TYPE**);
public:
	explicit Shader(ID3D11Device* pdevice, const wchar_t* filePath, const char* entryPoint = "main");
	~Shader();

	Shader(const Shader&) = delete;
	Shader(Shader&&) noexcept = delete;
	Shader& operator=(const Shader&) = delete;
	Shader& operator=(Shader&&) noexcept = delete;

	SHADER_TYPE* GetShader() const { return m_pShader; }
	ID3DBlob* GetShaderBlob() const { return m_pShaderBlob; }

private:
	SHADER_TYPE* m_pShader;
	ID3DBlob* m_pShaderBlob;

	HRESULT Init(ID3D11Device* pdevice, const wchar_t* filePath, const char* entryPoint);
};

template<typename SHADER_TYPE>
Shader<SHADER_TYPE>::Shader(ID3D11Device* pdevice, const wchar_t* filePath, const char* entryPoint)
	: m_pShader{ nullptr }
{
	const HRESULT res{ Init(pdevice, filePath, entryPoint) };
	if (FAILED(res))
		res;
}

template<typename SHADER_TYPE>
Shader<SHADER_TYPE>::~Shader()
{
	Helpers::SafeRelease(m_pShaderBlob);
	Helpers::SafeRelease(m_pShader);
}

template<typename SHADER_TYPE>
HRESULT Shader<SHADER_TYPE>::Init(ID3D11Device* pdevice, const wchar_t* filePath, const char* entryPoint)
{
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
		target = "vs_5_0";
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

	res = D3DCompileFromFile(filePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target.c_str(), flags, 0, &m_pShaderBlob, &perrorBlob);

	if (FAILED(res))
	{
		if (perrorBlob != nullptr)
		{
			char* errors = static_cast<char*>(perrorBlob->GetBufferPointer());

			const std::string errorString{ errors, perrorBlob->GetBufferSize() };

			OutputDebugStringA(errorString.c_str());
			Helpers::SafeRelease(perrorBlob);

			std::wcout << errorString.c_str() << std::endl;
			return res;
		}

		std::wstringstream ss;
		ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: ";
		ss << filePath;

		std::wcout << ss.str() << std::endl;
		return res;
	}

	return (pdevice->*shaderFnc)(m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), nullptr, &m_pShader);
}
