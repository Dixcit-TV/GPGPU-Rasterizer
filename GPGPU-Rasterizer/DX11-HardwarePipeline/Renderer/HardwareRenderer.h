#pragma once
#include <DirectXColors.h>
class Camera;
class Window;
class TriangleMesh;

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

	ID3D11Device* GetDevice() const { return m_pDxDevice; }
	ID3D11DeviceContext* GetDeviceContext() const { return m_pDxDeviceContext; }

	void Present() const { m_pDxSwapChain->Present(0, 0); }
	void ClearBuffers() const
	{
		m_pDxDeviceContext->ClearRenderTargetView(m_pRenderTargetView, reinterpret_cast<const float*>(&DirectX::Colors::Black));
		m_pDxDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	void DrawIndexed(Camera* pcamera, TriangleMesh* pmesh) const;

private:
	ID3D11Device* m_pDxDevice;
	ID3D11DeviceContext* m_pDxDeviceContext;
	IDXGISwapChain* m_pDxSwapChain;
	ID3D11RenderTargetView* m_pRenderTargetView;
	ID3D11DepthStencilView* m_pDepthStencilView;

	bool m_bInitialized;
};

