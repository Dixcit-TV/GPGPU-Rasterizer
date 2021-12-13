#pragma once
#include "WindowAndViewport/Window.h"

class HardwareRenderer
{
public:
	explicit HardwareRenderer();
	~HardwareRenderer();

	HardwareRenderer(const HardwareRenderer&) = delete;
	HardwareRenderer(HardwareRenderer&&) noexcept = delete;
	HardwareRenderer& operator=(const HardwareRenderer&) = delete;
	HardwareRenderer& operator=(HardwareRenderer&&) noexcept = delete;

	HRESULT Initialize(const Window& window);

private:
	ID3D11Device* pDxDevice;
	ID3D11DeviceContext* pDxDeviceContext;
	IDXGISwapChain* pDxSwapChain;
	ID3D11RenderTargetView* pRenderTargetView;
	ID3D11DepthStencilView* pDepthStencilView;

	bool bInitialized;
};

