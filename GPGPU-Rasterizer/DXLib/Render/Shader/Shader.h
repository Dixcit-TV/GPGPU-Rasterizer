#pragma once
#include <sstream>
#include <iostream>
#include <string>
#include "../../Common/Helpers.h"
#include "../../Managers/Logger.h"

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
	ID3D11ClassLinkage* GetClassLinkage() const { return m_ShaderLinkage; }

private:
	SHADER_TYPE* m_pShader;
	ID3DBlob* m_pShaderBlob;
	ID3D11ClassLinkage* m_ShaderLinkage;

	HRESULT Init(ID3D11Device* pdevice, const wchar_t* filePath, const char* entryPoint);
};

template<typename SHADER_TYPE>
Shader<SHADER_TYPE>::Shader(ID3D11Device* pdevice, const wchar_t* filePath, const char* entryPoint)
	: m_pShader{ nullptr }
{
	APP_LOG_IF_WARNING(SUCCEEDED(pdevice->CreateClassLinkage(&m_ShaderLinkage)), L"Class Linkage object could not be created for '" + std::wstring(filePath) + L"' !");
	APP_LOG_IF_WARNING(SUCCEEDED(Init(pdevice, filePath, entryPoint)), L"Shader '" + std::wstring(filePath) + L"' could not be loaded !");
}

template<typename SHADER_TYPE>
Shader<SHADER_TYPE>::~Shader()
{
	Helpers::SafeRelease(m_ShaderLinkage);
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

	return (pdevice->*shaderFnc)(m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), m_ShaderLinkage, &m_pShader);
}

using VertexShader = Shader<ID3D11VertexShader>;
using HullShader = Shader<ID3D11HullShader>;
using DomainShader = Shader<ID3D11DomainShader>;
using GeometryShader = Shader<ID3D11GeometryShader>;
using PixelShader = Shader<ID3D11PixelShader>;
using ComputeShader = Shader<ID3D11ComputeShader>;