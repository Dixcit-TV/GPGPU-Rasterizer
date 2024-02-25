#include "pch.h"
#include "CompuRenderer.h"

#include <iostream>
#include "WindowAndViewport/Window.h"
#include "Common/Helpers.h"
#include "../Mesh/Mesh.h"
#include "Managers/Logger.h"
#include "Pipeline/Pipeline.h"

namespace CompuRaster
{
	CompuRenderer::CompuRenderer()
		: m_pDxDevice{ nullptr }
		, m_pDxDeviceContext{ nullptr }
		, m_pDxSwapChain{ nullptr }
		, m_pRenderTargetUAV{ nullptr }
		, m_pDepthUAV{ nullptr }
		, m_bInitialized{ false }
	{}

	CompuRenderer::~CompuRenderer()
	{
		Helpers::SafeRelease(m_pDepthUAV);
		Helpers::SafeRelease(m_pRenderTargetUAV);
		Helpers::SafeRelease(m_pDxSwapChain);
		Helpers::SafeRelease(m_pDxDeviceContext);
		Helpers::SafeRelease(m_pDxDevice);
	}

	HRESULT CompuRenderer::Initialize(const Window& window)
	{
		HRESULT res{ S_OK };

		if (m_bInitialized)
			return res;

		APP_ASSERT_ERROR(window.IsInitialized(), L"Window not initialized !");

		UINT crDeviceFlag{ D3D11_CREATE_DEVICE_SINGLETHREADED };

#if defined(DEBUG) | defined(_DEBUG)
		crDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif 

		D3D_FEATURE_LEVEL featureLevel{ };

		res = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, crDeviceFlag, nullptr, 0, D3D11_SDK_VERSION, &m_pDxDevice, &featureLevel, &m_pDxDeviceContext);
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Could not create DX11 device !");
		APP_ASSERT_ERROR(featureLevel >= D3D_FEATURE_LEVEL_11_0, L"Supported feature level is below D3D_FEATURE_LEVEL_11_0 !");

		IDXGIDevice* dxgiDevice;
		res = m_pDxDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Could not query DXGI device !");

		IDXGIAdapter* dxgiAdapter;
		res = dxgiDevice->GetAdapter(&dxgiAdapter);
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Could not query DXGI adapter !");

		IDXGIFactory* dxgiFactory;
		res = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgiFactory));
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Could not query DXGI factory !");

		IDXGIOutput* tempOutput{};
		res = dxgiAdapter->EnumOutputs(0, &tempOutput);
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Failed to get temp IDXGIOutput from adapter !");

		UINT numOutput{ };
		res = tempOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, NULL, &numOutput, nullptr);
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Failed to request the number of Adapter outputs !");

		DXGI_MODE_DESC* bbDescs{ new DXGI_MODE_DESC[numOutput] };
		res = tempOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, NULL, &numOutput, bbDescs);
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Failed to request Adapter output list !");

		UINT bbDescIdx{ numOutput - 1 };

		for (UINT idx{}; idx < numOutput; ++idx)
		{
			const DXGI_MODE_DESC& desc{ bbDescs[idx] };
			if (desc.Width == window.GetWidth()
				&& desc.Height == window.GetHeight())
			{
				bbDescIdx = idx;
				break;
			}
		}

		Helpers::SafeRelease(tempOutput);

		DXGI_SWAP_CHAIN_DESC swapDesc{};
		swapDesc.BufferDesc = bbDescs[bbDescIdx];
		swapDesc.BufferCount = 1;
		swapDesc.BufferUsage = DXGI_USAGE_UNORDERED_ACCESS;
		swapDesc.SampleDesc.Count = 1;
		swapDesc.SampleDesc.Quality = 0;
		swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapDesc.Windowed = true;
		swapDesc.OutputWindow = window.GetHandle();

		delete[] bbDescs;

		res = dxgiFactory->CreateSwapChain(m_pDxDevice, &swapDesc, &m_pDxSwapChain);
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Failed to create swap chain !");

		ID3D11Texture2D* pbackBuffer;
		res = m_pDxSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pbackBuffer));
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Failed to query swap chain's back buffer !");

		D3D11_TEXTURE2D_DESC bbDesc{};
		pbackBuffer->GetDesc(&bbDesc);

		D3D11_UNORDERED_ACCESS_VIEW_DESC rtUAVDesc{};
		rtUAVDesc.Format = bbDesc.Format;
		rtUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		rtUAVDesc.Texture2D.MipSlice = 0;

		res = m_pDxDevice->CreateUnorderedAccessView(pbackBuffer, &rtUAVDesc, &m_pRenderTargetUAV);
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Failed to create render target UAV !");

		D3D11_TEXTURE2D_DESC depthStencilDesc{ bbDesc };
		depthStencilDesc.Format = DXGI_FORMAT_R32_FLOAT;

		ID3D11Texture2D* pdepthBuffer;
		res = m_pDxDevice->CreateTexture2D(&depthStencilDesc, nullptr, &pdepthBuffer);
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Failed to create depth buffer texture !");

		D3D11_UNORDERED_ACCESS_VIEW_DESC depthUAVDesc{};
		depthUAVDesc.Format = depthStencilDesc.Format;
		depthUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		depthUAVDesc.Texture2D.MipSlice = 0;

		res = m_pDxDevice->CreateUnorderedAccessView(pdepthBuffer, &depthUAVDesc, &m_pDepthUAV);
		APP_ASSERT_ERROR(SUCCEEDED(res), L"Failed to create depth buffer UAV !");

		Helpers::SafeRelease(pdepthBuffer);
		Helpers::SafeRelease(pbackBuffer);
		Helpers::SafeRelease(dxgiDevice);
		Helpers::SafeRelease(dxgiAdapter);
		Helpers::SafeRelease(dxgiFactory);

		m_bInitialized = true;

		APP_LOG_INFO(L"Renderer Initialized !");
		return res;
	}

	void CompuRenderer::Draw(Camera* pcamera, Mesh* pmesh) const
	{
		const UINT maxCompDispatch{ D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION };
		const UINT threadCount{ 1024 };
		const UINT triangleCount{ pmesh->GetIndexCount() / 3 };
		const UINT groupCount = static_cast<UINT>(ceil(triangleCount / static_cast<float>(threadCount)));
		const HelperStruct::Dispatch groupDim = { groupCount % maxCompDispatch, static_cast<UINT>(ceil(groupCount / static_cast<float>(maxCompDispatch))), 1 };

		pmesh->SetupDrawInfo(pcamera, m_pDxDeviceContext, groupDim);
		m_pDxDeviceContext->Dispatch(groupDim.x, groupDim.z, groupDim.y);
	}

	void CompuRenderer::DrawPipeline(const Pipeline& pipeline, Camera* pcamera, CompuMesh* pmesh) const
	{
		pipeline.Dispatch(m_pDxDeviceContext, pmesh, pcamera);
	}
}
