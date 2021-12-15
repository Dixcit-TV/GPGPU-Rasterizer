#include "pch.h"
#include "HardwareRenderer.h"

#include "../Mesh/TriangleMesh.h"
#include "Camera/Camera.h"
#include "Common/Helpers.h"
#include "WindowAndViewport/Window.h"

class TriangleMesh;

HardwareRenderer::HardwareRenderer()
	: m_pDxDevice{ nullptr }
	, m_pDxDeviceContext{ nullptr }
	, m_pDxSwapChain{ nullptr }
	, m_pRenderTargetView{ nullptr }
	, m_pDepthStencilView{ nullptr }
	, m_bInitialized{ false }
{}

HardwareRenderer::~HardwareRenderer()
{
	Helpers::SafeRelease(m_pDepthStencilView);
	Helpers::SafeRelease(m_pRenderTargetView);
	Helpers::SafeRelease(m_pDxSwapChain);
	Helpers::SafeRelease(m_pDxDeviceContext);
	Helpers::SafeRelease(m_pDxDevice);
}

HRESULT HardwareRenderer::Initialize(const Window& window)
{
	HRESULT res{ S_OK };

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

	//UINT m8xQuality{};
	//res = pDxDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 8, &m8xQuality);
	//assert(m8xQuality > 0);

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
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
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

	res = m_pDxDevice->CreateRenderTargetView(pbackBuffer, nullptr, &m_pRenderTargetView);
	if (FAILED(res)) return res;

	D3D11_TEXTURE2D_DESC depthStencilDesc{};
	pbackBuffer->GetDesc(&depthStencilDesc);
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D* pdepthStencilBuffer;
	res = m_pDxDevice->CreateTexture2D(&depthStencilDesc, nullptr, &pdepthStencilBuffer);
	if (FAILED(res)) return res;

	res = m_pDxDevice->CreateDepthStencilView(pdepthStencilBuffer, nullptr, &m_pDepthStencilView);
	if (FAILED(res)) return res;

	m_pDxDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

	D3D11_VIEWPORT viewPort{};
	viewPort.TopLeftX = 0.f;
	viewPort.TopLeftY = 0.f;
	viewPort.Width = window.GetWidth();
	viewPort.Height = window.GetHeight();
	viewPort.MinDepth = 0.f;
	viewPort.MaxDepth = 1.f;

	m_pDxDeviceContext->RSSetViewports(1, &viewPort);

	Helpers::SafeRelease(pdepthStencilBuffer);
	Helpers::SafeRelease(pbackBuffer);
	Helpers::SafeRelease(dxgiDevice);
	Helpers::SafeRelease(dxgiAdapter);
	Helpers::SafeRelease(dxgiFactory);

	m_bInitialized = true;
	return res;
}

void HardwareRenderer::DrawIndexed(Camera* pcamera, TriangleMesh* pmesh) const
{
	pmesh->SetupDrawInfo(pcamera, m_pDxDeviceContext);
	m_pDxDeviceContext->DrawIndexed(pmesh->GetIndexCount(), 0, 0);
}