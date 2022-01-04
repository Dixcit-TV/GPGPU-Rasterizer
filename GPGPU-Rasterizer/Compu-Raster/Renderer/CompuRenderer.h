#pragma once
#include <DirectXColors.h>

class Camera;
class Window;

namespace CompuRaster
{
	class CompuMesh;
	class Pipeline;
	class Mesh;

	class CompuRenderer
	{
	public:
		explicit CompuRenderer();
		~CompuRenderer();

		CompuRenderer(const CompuRenderer&) = delete;
		CompuRenderer(CompuRenderer&&) noexcept = delete;
		CompuRenderer& operator=(const CompuRenderer&) = delete;
		CompuRenderer& operator=(CompuRenderer&&) noexcept = delete;

		HRESULT Initialize(const Window& window);

		ID3D11Device* GetDevice() const { return m_pDxDevice; }
		ID3D11DeviceContext* GetDeviceContext() const { return m_pDxDeviceContext; }

		void Present() const { m_pDxSwapChain->Present(0, 0); }
		void ClearBuffers() const
		{
			m_pDxDeviceContext->ClearUnorderedAccessViewFloat(m_pRenderTargetUAV, reinterpret_cast<const float*>(&DirectX::Colors::Black));
			m_pDxDeviceContext->ClearUnorderedAccessViewFloat(m_pDepthUAV, reinterpret_cast<const float*>(&DirectX::Colors::White));

			ID3D11UnorderedAccessView* uavs[]{ m_pRenderTargetUAV, m_pDepthUAV };
			m_pDxDeviceContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
		}

		void Draw(Camera* pcamera, Mesh* pmesh) const;
		void DrawPipeline(const Pipeline& pipeline, Camera* pcamera, CompuMesh* pmesh) const;

	private:
		ID3D11Device* m_pDxDevice;
		ID3D11DeviceContext* m_pDxDeviceContext;
		IDXGISwapChain* m_pDxSwapChain;
		ID3D11UnorderedAccessView* m_pRenderTargetUAV;
		ID3D11UnorderedAccessView* m_pDepthUAV;

		bool m_bInitialized;
	};
}
