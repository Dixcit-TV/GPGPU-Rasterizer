#include "pch.h"
#include "CompuRenderer.h"

#include <iostream>

#include "WindowAndViewport/Window.h"
#include "Common/Helpers.h"

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

		if (!window.IsInitialized())
			return E_FAIL;

		UINT crDeviceFlag{ D3D11_CREATE_DEVICE_SINGLETHREADED };

#if defined(DEBUG) | defined(_DEBUG)
		crDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif 

		D3D_FEATURE_LEVEL featureLevel{ };

		res = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, crDeviceFlag, nullptr, 0, D3D11_SDK_VERSION, &m_pDxDevice, &featureLevel, &m_pDxDeviceContext);
		if (FAILED(res)) return res;
		if (featureLevel < D3D_FEATURE_LEVEL_11_0) return E_FAIL;

		IDXGIDevice* dxgiDevice;
		res = m_pDxDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (FAILED(res)) return res;

		IDXGIAdapter* dxgiAdapter;
		res = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapter));
		if (FAILED(res)) return res;

		IDXGIFactory* dxgiFactory;
		res = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgiFactory));
		if (FAILED(res)) return res;

		IDXGIOutput* tempOutput{};
		res = dxgiAdapter->EnumOutputs(0, &tempOutput);
		if (FAILED(res)) return res;

		UINT numOutput{ };
		res = tempOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, NULL, &numOutput, nullptr);
		if (FAILED(res)) return res;

		DXGI_MODE_DESC* bbDescs{ new DXGI_MODE_DESC[numOutput] };
		res = tempOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, NULL, &numOutput, bbDescs);
		if (FAILED(res)) return res;

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


		if (FAILED(res)) return res;
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
		if (FAILED(res)) return res;

		ID3D11Texture2D* pbackBuffer;
		res = m_pDxSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pbackBuffer));
		if (FAILED(res)) return res;

		D3D11_TEXTURE2D_DESC bbDesc{};
		pbackBuffer->GetDesc(&bbDesc);

		D3D11_UNORDERED_ACCESS_VIEW_DESC rtUAVDesc{};
		rtUAVDesc.Format = bbDesc.Format;
		rtUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		rtUAVDesc.Texture2D.MipSlice = 0;

		res = m_pDxDevice->CreateUnorderedAccessView(pbackBuffer, &rtUAVDesc, &m_pRenderTargetUAV);
		if (FAILED(res)) return res;

		D3D11_TEXTURE2D_DESC depthStencilDesc{ bbDesc };
		depthStencilDesc.Format = DXGI_FORMAT_R32_FLOAT;

		ID3D11Texture2D* pdepthBuffer;
		res = m_pDxDevice->CreateTexture2D(&depthStencilDesc, nullptr, &pdepthBuffer);
		if (FAILED(res)) return res;

		D3D11_UNORDERED_ACCESS_VIEW_DESC depthUAVDesc{};
		depthUAVDesc.Format = depthStencilDesc.Format;
		depthUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		depthUAVDesc.Texture2D.MipSlice = 0;

		res = m_pDxDevice->CreateUnorderedAccessView(pdepthBuffer, &depthUAVDesc, &m_pDepthUAV);
		if (FAILED(res)) return res;

		Helpers::SafeRelease(pdepthBuffer);
		Helpers::SafeRelease(pbackBuffer);
		Helpers::SafeRelease(dxgiDevice);
		Helpers::SafeRelease(dxgiAdapter);
		Helpers::SafeRelease(dxgiFactory);

		m_bInitialized = true;
		return res;
	}
}
